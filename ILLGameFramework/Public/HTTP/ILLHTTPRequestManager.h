// Copyright (C) 2015-2018 IllFonic, LLC.

#pragma once

#include "HAL/Event.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Http.h"
#include "Misc/SingleThreadRunnable.h"
#include "OnlineSubsystemTypes.h"

#include "ILLHTTPRequestTask.h"
#include "ILLHTTPRequestManager.generated.h"

ILLGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogILLHTTP, Log, All);

/**
 * @class UILLHTTPRequestManager
 * @brief Base manager for HTTP requests.
 */
UCLASS(Abstract, Transient, Config=Engine, Within=ILLGameInstance)
class ILLGAMEFRAMEWORK_API UILLHTTPRequestManager
: public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject interface
	virtual UWorld* GetWorld() const override;
	// End UObject interface

	/** Called from UILLGameInstance::Init. */
	virtual void Init();

	/** Called from UILLGameInstance::Shutdown. */
	virtual void Shutdown();

	/** @return Timer manager instance from the outer UILLGameInstance. */
	FTimerManager& GetTimerManager() const;

	//////////////////////////////////////////////////
	// Request task management
public:
	/**
	 * Cancels a pending or queued request task.
	 *
	 * @param RequestTask Managed HTTP request to cancel.
	 */
	virtual void CancelRequest(UILLHTTPRequestTask* RequestTask);

protected:
	friend class UILLHTTPRequestTask;

	/**
	 * Used by CreateRequest and sub-class variants to call PrepareRequest on the task being created.
	 *
	 * @param RequestTask Managed HTTP request to prepare.
	 */
	virtual bool PrepareRequest(UILLHTTPRequestTask* RequestTask);

	/**
	 * Called from UILLHTTPRequestTask::QueueRequest to add it to the pending task queue.
	 *
	 * @param RequestTask Managed HTTP request to queue.
	 */
	virtual void QueueRequest(UILLHTTPRequestTask* RequestTask);

	/**
	 * Called from UILLHTTPRequestTask::AsyncPrepareResponse to add it to the response queue.
	 *
	 * @param RequestTask Managed HTTP request to queue.
	 */
	virtual void QueueResponse(UILLHTTPRequestTask* RequestTask);

	/** Game thread ticker that is only active when response processing is needed. */
	virtual bool GameTaskTick(float DeltaTime);

	/** Called from our FILLHTTPRunnable to process asynchronous work. */
	virtual void AsyncTaskTick();

	// Delegate instance for GameTaskTick
	FTickerDelegate GameTickDelegate;

	// Delegate handle for GameTaskTick
	FDelegateHandle GameTickHandle;

	// Total time GameTaskTick has had nothing to do
	float GameTickIdleTime;

	// Maximum time that GameTaskTick will be idle before being shut off
	UPROPERTY(Config)
	float MaxGameTickIdleTime;

	// List of queued and processing tasks
	UPROPERTY(Transient)
	TArray<UILLHTTPRequestTask*> PendingTasks;

	// List of tasks pending cancellation
	TArray<UILLHTTPRequestTask*> CancelledTasks;

	// Critical section for CancelledTasks
	FCriticalSection CancelledTaskLock;

	// Pending request tasks from the game thread to ours
	TQueue<UILLHTTPRequestTask*> AsyncRequestTaskQueue;

	// Request tasks pending processing on our thread
	TArray<UILLHTTPRequestTask*> AsyncProcessTaskQueue;

	// Response tasks queued from the HTTP (game) thread and our own thread to ours
	TQueue<UILLHTTPRequestTask*, EQueueMode::Mpsc> AsyncResponseTaskQueue;

	// Response tasks queued from our thread to the game thread
	TQueue<UILLHTTPRequestTask*> GameResponseTaskQueue;

	// Maximum number of responses to process on the game thread per-tick, 0 for unlimited
	UPROPERTY(Config)
	uint32 MaxResponsesPerGameTick;

	//////////////////////////////////////////////////
	// Request thread
protected:
	/**
	 * @class FILLHTTPRunnable
	 */
	class FILLHTTPRunnable
	: public FRunnable
	, public FSingleThreadRunnable
	{
	public:
		FILLHTTPRunnable(UILLHTTPRequestManager* InManager, const TCHAR* ThreadName);
		~FILLHTTPRunnable();

		// Begin FRunnable interface
		virtual uint32 Run() override;
		virtual void Stop() override;
		virtual class FSingleThreadRunnable* GetSingleThreadInterface() override { return this; }
		// End FRunnable interface

		// Begin FSingleThreadRunnable interface
		virtual void Tick() override;
		// End FSingleThreadRunnable interface

		/** @return ThreadID of RunnableThread. */
		FORCEINLINE uint32 GetThreadID() const { return RunnableThread->GetThreadID(); }

		/**
		 * Wakes work thread up to process any request tasks.
		 *
		 * @param bFence Block the calling (game) thread until completion? When not on a multi-threaded platform, this just forcibly calls Tick.
		 */
		void TriggerWorkEvent(const bool bFence = false);

	protected:
		// Outer manager
		UILLHTTPRequestManager* Manager;

		// Runnable thread instance
		FRunnableThread* RunnableThread;

		// Event used to awaken the thread
		FEvent* WorkEvent;

		// Fence used to track work asynchronous tick completion
		FThreadSafeBool bWorkFence;

		// Do we want the thread to exit?
		bool bRequestingExit;
	};

	// Runnable thread container
	FILLHTTPRunnable* TaskThread;
};
