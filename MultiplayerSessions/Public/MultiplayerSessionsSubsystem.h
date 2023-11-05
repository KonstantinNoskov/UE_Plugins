// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MultiplayerSessionsSubsystem.generated.h"

/*
 * Declaring our own custom delegates for the Menu class to bind callbacks to 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionsComplete, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();
	
	// To handle session functionality. The Menu class will call these.
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();
	

#pragma region MENU DELEGATES
	
	// our own custom delegates for the Menu class to bind callback to
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionsComplete;
	FMultiplayerOnDestroySessionsComplete MultiplayerOnDestroySessionsComplete;
	FMultiplayerOnStartSessionsComplete MultiplayerOnStartSessionsComplete;

#pragma endregion


#pragma region ONLINEINTERFACE CALLBACKS FUNCTIONS
	
protected:
	
	// Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	// These don't need to be called outside this class.
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

#pragma endregion
	
private:

	// Stores reference to a Multiplayer session interface
	IOnlineSessionPtr SessionInterface;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

#pragma region ONLINEINTERFACE DELEGATES
	/*
	 * To add to the Online Session Interface delegate list.
	 * We'll bind our MultiplayerSessionsSubsystem internal call backs to these.
	 */
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	FOnFindSessionsCompleteDelegate	FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	
	FOnJoinSessionCompleteDelegate	JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	FOnStartSessionCompleteDelegate	StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{ false };
	int32 LastNumPublicConnections;
	FString LastMatchType;
 
#pragma endregion
	
};
