// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Network")
	void MenuSetup(int32 NumberOfPublicConnection = 4, FString TypeOfMatch = FString(TEXT("Noskov")), FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/L_Lobby?listen")));

protected:
	
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

	UFUNCTION()
	void HostButtonClicked();
	
	UFUNCTION()
	void JoinButtonClicked();

	UFUNCTION()
	void QuitButtonClicked();

	void MenuTearDown();

	// Reference to a subsystem designed to handle all online session functionality
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	// Menu setup properties
	int32 NumPublicConnections{4};
	FString MatchType{TEXT("Noskov")};
	
	FString PathToLobby{TEXT("")};
};
