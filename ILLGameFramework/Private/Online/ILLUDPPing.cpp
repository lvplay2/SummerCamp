// Copyright (C) 2015-2018 IllFonic, LLC.

#include "ILLGameFramework.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

#include "ILLBuildDefines.h"
#include "ILLUDPPing.h"
#include "SHA256.h"

/**
 * Copy of UE4's UDPPing with a few critical differences:
 * - Using SHA256 for the checksum/hash instead of the one's complement checksum used by UE4's.
 * - Only processing responses that are exactly the size expected.
 * - Randomizing the sequence number sent.
 * - Keep polling for a valid response until a timeout hits, so invalid responses are skipped.
 * - New magic numbers.
 */

#define MAGIC_HIGH ILLBUILD_BUILD_NUMBER
#define MAGIC_LOW 0x24357721

bool ResolveAddress(ISocketSubsystem* SocketSub, const FString& HostName, FString& OutIp)
{
	//ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSub)
	{
		TSharedRef<FInternetAddr> HostAddr = SocketSub->CreateInternetAddr();
		ESocketErrors HostResolveError = SocketSub->GetHostByName(TCHAR_TO_ANSI(*HostName), *HostAddr);
		if (HostResolveError == SE_NO_ERROR || HostResolveError == SE_EWOULDBLOCK)
		{
			OutIp = HostAddr->ToString(false);
			return true;
		}
	}

	return false;
}

FIcmpEchoResult ILLUDPEchoImpl(ISocketSubsystem* SocketSub, const FString& TargetAddress, float Timeout)
{
	struct FUDPPingHeader
	{
		uint16 Id;
		uint16 Sequence;
		uint8 Hash[FSHA256::HASH_BYTES];
	};

	// Size of the udp header sent/received
	static const SIZE_T UDPPingHeaderSize = sizeof(FUDPPingHeader);

	// The packet we send is just the header plus our payload
	static const SIZE_T PacketSize = UDPPingHeaderSize + sizeof(uint32)*2 + sizeof(uint64);

	// Location of the timecode in the buffer
	static const SIZE_T TimeCodeOffset = UDPPingHeaderSize;

	// Location of the payload in the buffer
	static const SIZE_T MagicNumberOffset = TimeCodeOffset + sizeof(uint64);

	FIcmpEchoResult Result;
	Result.Status = EIcmpResponseStatus::InternalError;

	FString PortStr;

	TArray<FString> IpParts;
	int32 NumTokens = TargetAddress.ParseIntoArray(IpParts, TEXT(":"));

	FString Address = TargetAddress;
	if (NumTokens == 2)
	{
		Address = IpParts[0];
		PortStr = IpParts[1];
	}

	FString ResolvedAddress;
	if (!ResolveAddress(SocketSub, Address, ResolvedAddress))
	{
		Result.Status = EIcmpResponseStatus::Unresolvable;
		return Result;
	}

	int32 Port = 0;
	Lex::FromString(Port, *PortStr);

	if (SocketSub)
	{
		FSocket* Socket = SocketSub->CreateSocket(NAME_DGram, TEXT("UDPPing"), false);
		if (Socket)
		{
			uint8 SendBuffer[PacketSize];
			// Clear packet buffer
			FMemory::Memset(SendBuffer, 0, PacketSize);

			Result.ResolvedAddress = ResolvedAddress;

			TSharedRef<FInternetAddr> ToAddr = SocketSub->CreateInternetAddr();

			bool bIsValid = false;
			ToAddr->SetIp(*ResolvedAddress, bIsValid);
			ToAddr->SetPort(Port);
			if (bIsValid)
			{
				uint16 SentId = (uint16)FPlatformProcess::GetCurrentProcessId();
				uint16 SentSeq = FMath::Rand();

				FUDPPingHeader* PacketHeader = reinterpret_cast<FUDPPingHeader*>(SendBuffer);
				PacketHeader->Id = SentId;
				PacketHeader->Sequence = SentSeq;
				FMemory::Memzero(PacketHeader->Hash);

				// Put some data into the packet payload
				uint32* PayloadStart = (uint32*)(SendBuffer + MagicNumberOffset);
				PayloadStart[0] = MAGIC_HIGH;
				PayloadStart[1] = MAGIC_LOW;

				// Calculate the time packet is to be sent
				uint64* TimeCodeStart = (uint64*)(SendBuffer + TimeCodeOffset);
				FDateTime TimeCode = FDateTime::UtcNow();
				TimeCodeStart[0] = TimeCode.GetTicks();

				// Calculate the packet hash
				FSHA256::Hash(SendBuffer, PacketSize, PacketHeader->Hash);

				uint8 ResultBuffer[PacketSize];

				double TimeLeft = Timeout;
				double StartTime = FPlatformTime::Seconds();

				int32 BytesSent = 0;
				if (Socket->SendTo(SendBuffer, PacketSize, BytesSent, *ToAddr))
				{
					bool bDone = false;
					while (!bDone)
					{
						if (TimeLeft > 0.0 && Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(TimeLeft)))
						{
							double EndTime = FPlatformTime::Seconds();
							TimeLeft = FPlatformMath::Max(0.0, (double)Timeout - (EndTime - StartTime));

							int32 BytesRead = 0;
							TSharedRef<FInternetAddr> RecvAddr = SocketSub->CreateInternetAddr();
							if (Socket->RecvFrom(ResultBuffer, PacketSize, BytesRead, *RecvAddr) && BytesRead == PacketSize)
							{
								FDateTime NowTime = FDateTime::UtcNow();

								Result.ReplyFrom = RecvAddr->ToString(false);
								FUDPPingHeader* RecvHeader = reinterpret_cast<FUDPPingHeader*>(ResultBuffer);

								// Validate the packet hash
								uint8 RecvHash[FSHA256::HASH_BYTES];
								FMemory::Memcpy(RecvHash, RecvHeader->Hash);
								FMemory::Memzero(RecvHeader->Hash);
								uint8 LocalHash[FSHA256::HASH_BYTES];
								FSHA256::Hash((uint8*)RecvHeader, PacketSize, LocalHash);

								if (FMemory::Memcmp(RecvHash, LocalHash, FSHA256::HASH_BYTES) == 0)
								{
									// Convert values back from network byte order
									RecvHeader->Id = RecvHeader->Id;
									RecvHeader->Sequence = RecvHeader->Sequence;

									uint32* MagicNumberPtr = (uint32*)(ResultBuffer + MagicNumberOffset);
									if (MagicNumberPtr[0] == MAGIC_HIGH && MagicNumberPtr[1] == MAGIC_LOW)
									{
										// Estimate elapsed time
										uint64* TimeCodePtr = (uint64*)(ResultBuffer + TimeCodeOffset);
										FDateTime PrevTime(*TimeCodePtr);
										double DeltaTime = (NowTime - PrevTime).GetTotalSeconds();

										if(Result.ReplyFrom == Result.ResolvedAddress
										&& RecvHeader->Id == SentId
										&& RecvHeader->Sequence == SentSeq
										&& DeltaTime >= 0.0 && DeltaTime < (60.0 * 1000.0))
										{
											Result.Time = DeltaTime;
											Result.Status = EIcmpResponseStatus::Success;
											bDone = true;
										}
									}
								}
							}
						}
						else
						{
							// timeout
							Result.Status = EIcmpResponseStatus::Timeout;
							Result.ReplyFrom.Empty();
							Result.Time = Timeout;

							bDone = true;
						}
					}
				}
			}

			SocketSub->DestroySocket(Socket);
		}
	}

	return Result;
}

class FUDPPingAsyncResult
: public FTickerObjectBase
{
public:
	FUDPPingAsyncResult(ISocketSubsystem* InSocketSub, const FString& TargetAddress, float Timeout, uint32 StackSize, FIcmpEchoResultCallback InCallback)
	: FTickerObjectBase(0)
	, SocketSub(InSocketSub)
	, Callback(InCallback)
	, bThreadCompleted(false)
	{
		if (SocketSub)
		{
			bThreadCompleted = false;
			TFunction<FIcmpEchoResult()> Task = [this, TargetAddress, Timeout]()
			{
				auto Result = ILLUDPEchoImpl(SocketSub, TargetAddress, Timeout);
				bThreadCompleted = true;
				return Result;
			};

			FutureResult = AsyncThread(Task, StackSize);
		}
		else
		{
			bThreadCompleted = true;
		}
	}

	virtual ~FUDPPingAsyncResult()
	{
		check(IsInGameThread());

		if (FutureResult.IsValid())
		{
			FutureResult.Wait();
		}
	}

private:
	virtual bool Tick(float DeltaTime) override
	{
		if (bThreadCompleted)
		{
			FIcmpEchoResult Result;
			if (FutureResult.IsValid())
			{
				Result = FutureResult.Get();
			}

			Callback(Result);

			delete this;
			return false;
		}
		return true;
	}

	/** Reference to the socket subsystem */
	ISocketSubsystem* SocketSub;
	/** Callback when the ping result returns */
	FIcmpEchoResultCallback Callback;
	/** Thread task complete */
	FThreadSafeBool bThreadCompleted;
	/** Async result future */
	TFuture<FIcmpEchoResult> FutureResult;
};

void ILLUDPEcho(const FString& TargetAddress, float Timeout, FIcmpEchoResultCallback HandleResult)
{
	int32 StackSize = 0;

#if PLATFORM_WINDOWS
	GConfig->GetInt(TEXT("Ping"), TEXT("StackSize"), StackSize, GEngineIni);

	// Sanity clamp
	if (StackSize != 0)
	{
		StackSize = FMath::Max<int32>(FMath::Min<int32>(StackSize, 2 * 1024 * 1024), 32 * 1024);
	}
#endif

	ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	new FUDPPingAsyncResult(SocketSub, TargetAddress, Timeout, StackSize, HandleResult);
}
