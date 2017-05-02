// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved
// Plugin written by Philipp Buerki. Copyright 2017. All Rights reserved..

#include "OnlineError.h"
#include "OnlineSubsystemB3atZTypes.h"

//static
const FString FOnlineError::GenericErrorCode = TEXT("GenericError");

FOnlineError::FOnlineError()
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(0)
{
}

FOnlineError::FOnlineError(bool bSucceededIn)
	: bSucceeded(bSucceededIn)
	, HttpResult(0)
	, NumericErrorCode(0)
{
}

FOnlineError::FOnlineError(const FString& ErrorCodeIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(0)
{
	SetFromErrorCode(ErrorCodeIn);
}

void FOnlineError::SetFromErrorCode(const FString& ErrorCodeIn)
{
	ErrorCode = ErrorCodeIn;
	ErrorRaw = ErrorCodeIn;
}

FOnlineError::FOnlineError(const FText& ErrorMessageIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(INDEX_NONE)
{
	SetFromErrorMessage(ErrorMessageIn);
}

void FOnlineError::SetFromErrorMessage(const FText& ErrorMessageIn)
{
	ErrorMessage = ErrorMessageIn;
	ErrorCode = FTextInspector::GetKey(ErrorMessageIn).Get(GenericErrorCode);
	ErrorRaw = ErrorMessageIn.ToString();
}

const TCHAR* FOnlineError::ToLogString() const
{
	if (!ErrorMessage.IsEmpty())
	{
		return *ErrorMessage.ToString();
	}
	else if (!ErrorCode.IsEmpty())
	{
		return *ErrorCode;
	}
	else if (!ErrorRaw.IsEmpty())
	{
		return *ErrorRaw;
	}
	else
	{
		return TEXT("(Empty FOnlineError)");
	}
}

EB3atZOnlineServerConnectionStatus::Type FOnlineError::GetConnectionStatusFromHttpResult() const
{
	if (bSucceeded)
	{
		return EB3atZOnlineServerConnectionStatus::Connected;
	}

	switch (HttpResult)
	{
	case 0:
		// no response means we couldn't even connect
		return EB3atZOnlineServerConnectionStatus::ConnectionDropped;
	case 401:
		// No auth at all
		return EB3atZOnlineServerConnectionStatus::InvalidUser;
	case 403:
		// Auth failure
		return EB3atZOnlineServerConnectionStatus::NotAuthorized;
	case 503:
	case 504:
		// service is not avail
		return EB3atZOnlineServerConnectionStatus::ServersTooBusy;
	case 505:
		// update to supported version
		return EB3atZOnlineServerConnectionStatus::UpdateRequired;
	case 408:
	case 501:
	case 502:
		// other bad responses (ELB, etc)
		return EB3atZOnlineServerConnectionStatus::ServiceUnavailable;
	default:
		// this is a domain specific error like a 400 or 404, this will be handled by application code
		return EB3atZOnlineServerConnectionStatus::Connected;
	}
}
