// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
	
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if(Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	// check if valid
	if(!SessionInterface.IsValid()) { return; }

	// set reference to current session
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);

	// if there's current session, destroy it
	if(ExistingSession)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}
	
	// Add @var CreateSessionCompleteDelegate to the delegate list and store it in @var CreateSessionCompleteDelegateHandle
	// so we can delete it from the list later
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// Set session settings
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	// if session wasn't created by any reason...
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{	
		// clear OnCreateSessionCompleteDelegate from DelegateHandleList
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
	
}
void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	// check if SessionInterface is valid 
	if(!SessionInterface.IsValid()) { return; }

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());

	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	// Create reference to local player 
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// if session wasn't found...
	if(!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		// remove OnFindSessionsDelegateHandle from OnlineInterface delegate list...
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		
		// broadcast MultiplayerOnFindSessionsCompleteDelegate to execute callbacks in Menu class.
		// Pass an empty array because we haven't found any session and return false
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}
void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	// check if SessionInterface is valid 
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Add JoinSessionCompleteDelegate to OnlineInterface delegate list  
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	// get reference to a local player
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// if we can't join session for some reason...
	if(!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		// remove JoinSessionCompleteDelegate from OnlineInterface delegate list
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		// pass in to Menu class that join to session is failed and pass result status
		MultiplayerOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
	
}
void UMultiplayerSessionsSubsystem::DestroySession()
{
	if(SessionInterface)
	{
		MultiplayerOnDestroySessionsComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	
	if(!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		MultiplayerOnDestroySessionsComplete.Broadcast(false);
	}
}
void UMultiplayerSessionsSubsystem::StartSession()
{
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if(SessionInterface)
	{
		// clear OnCreateSessionCompleteDelegate from DelegateHandleList
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(true);
	}
}
void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if(SessionInterface)
	{
		// remove OnFindSessionsDelegateHandle from OnlineInterface delegate list...
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// if sessions was found but session array is empty somehow
	// pass in an empty array and return false
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);

		// call return here so we can't reach "successful" block 
		return;
	}
	
	// broadcast MultiplayerOnFindSessionsCompleteDelegate to execute callbacks in Menu class
	// pass an array with found sessions and return true
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
	
}
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// check if SessionInterface is valid 
	if (SessionInterface)
	{
		// remove JoinSessionCompleteDelegate from OnlineInterface delegate list
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		// pass in to Menu class that the join to a session was successful
		MultiplayerOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::Success);
	}

	// Add JoinSessionCompleteDelegate to OnlineInterface delegate list  
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
}
void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// check if SessionInterface is valid 
	if(SessionInterface)
	{
		// remove DestroyDelegate from delegate list
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;

		CreateSession(LastNumPublicConnections, LastMatchType);
	}

	MultiplayerOnDestroySessionsComplete.Broadcast(bWasSuccessful);
	
}
void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}




