#include "Menu.h"

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"


void UMenu::MenuSetup(int32 NumberOfPublicConnection, FString TypeOfMatch, FString LobbyPath)
{
	NumPublicConnections = NumberOfPublicConnection;
	MatchType = TypeOfMatch;
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();

	if(World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();

		if(PlayerController)
		{
			// Set input settings
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	
	if(MultiplayerSessionsSubsystem)
	{
		// Bind callback functions to a MultiplayerSubsystem's delegates.
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionsComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionsComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionsComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	if(!Super::Initialize()) { return false; }

	if(HostButton) { HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked); }
	if(JoinButton) { JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked); }
	if(QuitButton) { QuitButton->OnClicked.AddDynamic(this, &ThisClass::QuitButtonClicked); }
	
	return true;
}

/*
 * Will be called after we travel to another level
 */
void UMenu::NativeDestruct()
{
	// this function return back input to a character controller so we can move it
	// also hide the mouse cursor
	MenuTearDown();
	
	Super::NativeDestruct();
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();

	if(World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if(PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}


/* BUTTONS FUNCTIONS
 * ====================================================================================================================
 * 
 *	This functions contains logic which is called every time the buttons is clicked.
 * 
 * Button clicked functions should be added dynamically to OnClick delegate in UMenu::Initialize() function
 * The button function name must comply with the following format : void <SomeAction>ButtonClicked() 
 *
 * Example:
 * if(<SomeAction>Button) { <SomeAction>Button->OnClicked.AddDynamic(this, &ThisClass::<SomeAction>ButtonClicked); }
 * 
 * ====================================================================================================================
 */
void UMenu::HostButtonClicked()
{
	// Disable button on click immediately by default regardless of whether was host successful
	HostButton->SetIsEnabled(false);

	// MultiplayerSessionsSubsystem valid check 
	if(MultiplayerSessionsSubsystem)
	{
		// Call CreateSession() on SessionSubsystem 
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}
void UMenu::JoinButtonClicked()
{
	// Disable button on click immediately by default regardless of whether was join successful
	JoinButton->SetIsEnabled(false);

	// MultiplayerSessionsSubsystem valid check 
	if(MultiplayerSessionsSubsystem)
	{
		// Call FindSessions() on SessionSubsystem 
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}
void UMenu::QuitButtonClicked()
{
	// Get references to world and player controller instances
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World->GetFirstPlayerController();

	// World and PlayerController valid check
	if(!World || !PlayerController) return;

	// quit game
	UKismetSystemLibrary::QuitGame(World, PlayerController, EQuitPreference::Quit,true);
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	// if
	if(bWasSuccessful)
	{	
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Green,FString::Printf(TEXT("Session was successfully created!"))); }

		UWorld* World = GetWorld();

		if(World)
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString::Printf(TEXT("Failed to create session!"))); }
		HostButton->SetIsEnabled(true);
	}
}
void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	// check if MultiplayerSessionsSubsystem is valid 
	if(!MultiplayerSessionsSubsystem) { return; }

	// 
	for (FOnlineSessionSearchResult Result : SessionResults)
	{
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);

		if (SettingsValue == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}

	// if we don't find the session
	// or for some reason, session was found but sessions array is empty
	if(!bWasSuccessful || !SessionResults.Num())
	{
		// enable join button back so we can try to find session again
		JoinButton->SetIsEnabled(true);
	}
}


void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if(Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			FString Address;

			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			// Reference to a local player controller
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, TRAVEL_Absolute);
			}
		}
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
	
}
void UMenu::OnDestroySession(bool bWasSuccessful)
{
}
void UMenu::OnStartSession(bool bWasSuccessful)
{
}
