// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"
#include "ILLHTTPRequestManager.h"

#include "OnlineSubsystemUtils.h"

#include "ILLGameInstance.h"
#include "ILLHTTPRequestTask.h"

DEFINE_LOG_CATEGORY(LogILLHTTP);

UILLHTTPRequestManager::UILLHTTPRequestManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MaxGameTickIdleTime = 1.f;
	MaxResponsesPerGameTick = 1;
}

UWorld* UILLHTTPRequestManager::GetWorld() const
{
	return GetOuterUILLGameInstance()->GetWorld();
}

void UILLHTTPRequestManager::Init()
{
	// Create a game thread tick delegate
	GameTickDelegate = FTickerDelegate::CreateUObject(this, &ThisClass::GameTaskTick);

	// Create the async task thread
	TaskThread = new FILLHTTPRunnable(this, *GetName());
}

void UILLHTTPRequestManager::Shutdown()
{
	// Cleanup the async task thread
	if (TaskThread)
	{
		delete TaskThread;
		TaskThread = nullptr;
	}
}

FTimerManager& UILLHTTPRequestManager::GetTimerManager() const
{
	return GetOuterUILLGameInstance()->GetTimerManager();
}

void UILLHTTPRequestManager::CancelRequest(UILLHTTPRequestTask* RequestTask)
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
	check(RequestTask->HasBeenQueued());

	// Only bother when not already flagged for cancellation
	if (RequestTask->bCanceled)
		return;

	// Flag as canceled
	RequestTask->bCanceled = true;

	// Add it to the cancellation list
	{
		FScopeLock Lock(&CancelledTaskLock);
		CancelledTasks.Add(RequestTask);
	}

	// Wake the thread
	TaskThread->TriggerWorkEvent();
}

bool UILLHTTPRequestManager::PrepareRequest(UILLHTTPRequestTask* RequestTask)
{
	if (!RequestTask->PrepareRequest())
	{
		UE_LOG(LogILLHTTP, Warning, TEXT("%s: %s failed to PrepareRequest!"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());
		return false;
	}

	return true;
}

void UILLHTTPRequestManager::QueueRequest(UILLHTTPRequestTask* RequestTask)
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());

	check(IsInGameThread());
	check(IsValid(RequestTask));

	// Register for ticker callback
	if (!GameTickHandle.IsValid())
	{
		GameTickHandle = FTicker::GetCoreTicker().AddTicker(GameTickDelegate, 0.0f);
	}
	GameTickIdleTime = 0.f;

	// Track in a list to keep from being GC'ed
	PendingTasks.AddUnique(RequestTask);

	// Add it to the request queue and wake the thread
	AsyncRequestTaskQueue.Enqueue(RequestTask);
	TaskThread->TriggerWorkEvent();
}

void UILLHTTPRequestManager::QueueResponse(UILLHTTPRequestTask* RequestTask)
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s: %s"), ANSI_TO_TCHAR(__FUNCTION__), *RequestTask->GetDebugName());

	// Called from the game thread or from our thread via UILLHTTPRequestTask::OnProcessRequestComplete
	check(IsInGameThread() || FPlatformTLS::GetCurrentThreadId() == TaskThread->GetThreadID());

	// Add it to the response queue and wake the thread
	AsyncResponseTaskQueue.Enqueue(RequestTask);
	TaskThread->TriggerWorkEvent();
}

bool UILLHTTPRequestManager::GameTaskTick(float DeltaTime)
{
	// Optionally process a limited number of responses per game tick
	{
		UILLHTTPRequestTask* RequestTask = nullptr;
		uint32 NumResponsesProcessed = 0;
		while ((MaxResponsesPerGameTick <= 0 || NumResponsesProcessed < MaxResponsesPerGameTick) && GameResponseTaskQueue.Dequeue(RequestTask))
		{
			// Game thread response handling, remove it from PendingTasks after
			RequestTask->HandleResponse();
			PendingTasks.Remove(RequestTask);
			++NumResponsesProcessed;
		}
	}

	// Throttle DeltaTime so frame spikes do not cause false timeouts
	const float ClampedDeltaTime = FMath::Min(DeltaTime, .5f);

	if (PendingTasks.Num() != 0)
	{
		GameTickIdleTime = 0.f;

		// Check processing tasks when not loading a map
		UWorld* World = GetWorld();
		if (World && !World->IsInSeamlessTravel())
		{
			for (int32 TaskIndex = 0; TaskIndex < PendingTasks.Num(); ++TaskIndex)
			{
				UILLHTTPRequestTask* RequestTask = PendingTasks[TaskIndex];
				if (RequestTask->bProcessing)
				{
					// Track processing time
					RequestTask->ProcessingTime += ClampedDeltaTime;

					// Check for timeout
					if (RequestTask->TimeoutDuration > 0.f && RequestTask->ProcessingTime >= RequestTask->TimeoutDuration && !RequestTask->bTimedOut)
					{
						RequestTask->bTimedOut = true;
						CancelRequest(RequestTask);
						continue;
					}
				}
				else
				{
					check(RequestTask->bQueued);

					// Track queued time
					RequestTask->QueuedTime += ClampedDeltaTime;

					// When ready to be processed, wake the thread
					if (RequestTask->IsReadyToProcess())
					{
						TaskThread->TriggerWorkEvent();
					}
				}
			}
		}
	}
	else
	{
		GameTickIdleTime += ClampedDeltaTime;

		// Cleanup the ticker callback when there is nothing to do
		if (GameTickIdleTime >= MaxGameTickIdleTime && ensure(GameTickHandle.IsValid()))
		{
			FTicker::GetCoreTicker().RemoveTicker(GameTickHandle);
			GameTickHandle.Reset();
		}
	}

	return true;
}

void UILLHTTPRequestManager::AsyncTaskTick()
{
	// Prepare all pending requests
	{
		UILLHTTPRequestTask* RequestTask = nullptr;
		while (AsyncRequestTaskQueue.Dequeue(RequestTask))
		{
			RequestTask->AsyncPrepareRequest();
			AsyncProcessTaskQueue.Add(RequestTask);
		}
	}

	// Process any ready tasks
	{
		for (int32 Index = 0; Index < AsyncProcessTaskQueue.Num();)
		{
			UILLHTTPRequestTask* RequestTask = AsyncProcessTaskQueue[Index];
			if (RequestTask->IsReadyToProcess())
			{
				RequestTask->AsyncProcessRequest();
				AsyncProcessTaskQueue.RemoveAt(Index);
			}
			else
			{
				++Index;
			}
		}
	}

	// Cancel requests that need it
	// Done after processing requests so the engine's HTTP backend is happy
	if (CancelledTasks.Num() > 0)
	{
		FScopeLock Lock(&CancelledTaskLock);
		for (UILLHTTPRequestTask* RequestTask : CancelledTasks)
		{
			if (RequestTask->bProcessing)
			{
				// Already processing, follow normal cancellation path
				// This should fire UILLHTTPRequestTask::OnProcessRequestComplete which will call QueueResponse
				RequestTask->HttpRequest->CancelRequest();
			}
			else
			{
				// Not processing, kick back to the game thread
				AsyncProcessTaskQueue.Remove(RequestTask);
				GameResponseTaskQueue.Enqueue(RequestTask);
			}
		}
		CancelledTasks.Empty();
	}

	// Finish async processing and hand responses back to the game thread
	{
		UILLHTTPRequestTask* RequestTask = nullptr;
		while (AsyncResponseTaskQueue.Dequeue(RequestTask))
		{
			RequestTask->AsyncPrepareResponse();
			GameResponseTaskQueue.Enqueue(RequestTask);
		}
	}
}

UILLHTTPRequestManager::FILLHTTPRunnable::FILLHTTPRunnable(UILLHTTPRequestManager* InManager, const TCHAR* ThreadName)
: Manager(InManager)
, bRequestingExit(false)
{
	// Create the WorkEvent before the RunnableThread so we don't crash
	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
	RunnableThread = FRunnableThread::Create(this, ThreadName, 0, TPri_BelowNormal);
}

UILLHTTPRequestManager::FILLHTTPRunnable::~FILLHTTPRunnable()
{
	// Deleting the runnable will cause it to call Stop, allowing the thread to exit cleanly
	delete RunnableThread;

	// Cleanup the WorkEvent after the thread is gone
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;
}

uint32 UILLHTTPRequestManager::FILLHTTPRunnable::Run()
{
	do 
	{
		// Wait infinitely to be woken up before ticking
		WorkEvent->Wait();
		if (!bRequestingExit)
			Tick();

		bWorkFence = false;
	} while (!bRequestingExit);

	return 0;
}

void UILLHTTPRequestManager::FILLHTTPRunnable::Stop()
{
	UE_LOG(LogILLHTTP, Verbose, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));

	// Let the thread exit
	bRequestingExit = true;
	TriggerWorkEvent(true);
}

void UILLHTTPRequestManager::FILLHTTPRunnable::Tick()
{
	Manager->AsyncTaskTick();
}

void UILLHTTPRequestManager::FILLHTTPRunnable::TriggerWorkEvent(const bool bFence/* = false*/)
{
	if (FPlatformProcess::SupportsMultithreading())
	{
		if (bFence)
		{
			check(FPlatformTLS::GetCurrentThreadId() != GetThreadID());
			check(!bWorkFence);
			bWorkFence = true;
		}

		WorkEvent->Trigger();

		while (bWorkFence)
		{
			FPlatformProcess::Sleep(.2f);
		}
	}
	else if (bFence)
	{
		Tick();
	}
}
