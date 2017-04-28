// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "../OnlineSubsystemB3atZUtils/Public/B3atZPartyBeaconState.h"
#include "../OnlineSubsystemB3atZUtils/Public/B3atZOnlineBeacon.h"

namespace ETeamAssignmentMethod
{
	const FName Smallest = FName(TEXT("Smallest"));
	const FName BestFit = FName(TEXT("BestFit"));
	const FName Random = FName(TEXT("Random"));
}

bool FB3atZPartyReservation::IsValid() const
{
	bool bIsValid = false;
	if (PartyLeader.IsValid() && PartyMembers.Num() >= 1)
	{
		bIsValid = true;
		for (const FB3atZPlayerReservation& PlayerRes : PartyMembers)
		{
			if (!PlayerRes.UniqueId.IsValid())
			{
				bIsValid = false;
				break;
			}

			if (PartyLeader == PlayerRes.UniqueId &&
				PlayerRes.ValidationStr.IsEmpty())
			{
				bIsValid = false;
				break;
			}
		}
	}

	return bIsValid;
}

UB3atZPartyBeaconState::UB3atZPartyBeaconState(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	SessionName(NAME_None),
	NumConsumedReservations(0),
	MaxReservations(0),
	NumTeams(0),
	NumPlayersPerTeam(0),
	TeamAssignmentMethod(ETeamAssignmentMethod::Smallest),
	ReservedHostTeamNum(0),
	ForceTeamNum(0)
{
}

bool UB3atZPartyBeaconState::InitState(int32 InTeamCount, int32 InTeamSize, int32 InMaxReservations, FName InSessionName, int32 InForceTeamNum)
{
	if (InMaxReservations > 0)
	{
		SessionName = InSessionName;
		NumTeams = InTeamCount;
		NumPlayersPerTeam = InTeamSize;
		MaxReservations = InMaxReservations;
		ForceTeamNum = InForceTeamNum;
		Reservations.Empty(MaxReservations);

		InitTeamArray();
		return true;
	}

	return false;
}

void UB3atZPartyBeaconState::InitTeamArray()
{
	if (NumTeams > 1)
	{
		// Grab one for the host team
		ReservedHostTeamNum = FMath::Rand() % NumTeams;
	}
	else
	{
		// Only one team, so choose 'forced team' for everything
		ReservedHostTeamNum = ForceTeamNum;
	}

	UE_LOG(LogBeacon, Display,
		TEXT("Beacon State: team count (%d), team size (%d), host team (%d)"),
		NumTeams,
		NumPlayersPerTeam,
		ReservedHostTeamNum);
}

bool UB3atZPartyBeaconState::ReconfigureTeamAndPlayerCount(int32 InNumTeams, int32 InNumPlayersPerTeam, int32 InNumReservations)
{
	bool bSuccess = false;

	//Check total existing reservations against new total maximum
	if (NumConsumedReservations <= InNumReservations)
	{
		bool bTeamError = false;
		// Check teams with reservations against new team count
		if (NumTeams > InNumTeams)
		{
			// Any team about to be removed can't have players already there
			for (int32 TeamIdx = InNumTeams; TeamIdx < NumTeams; TeamIdx++)
			{
				if (GetNumPlayersOnTeam(TeamIdx) > 0)
				{
					bTeamError = true;
					UE_LOG(LogBeacon, Warning, TEXT("Beacon has players on a team about to be removed."));
				}
			}
		}

		bool bTeamSizeError = false;
		// Check num players per team against new team size
		if (NumPlayersPerTeam > InNumPlayersPerTeam)
		{
			for (int32 TeamIdx = 0; TeamIdx<NumTeams; TeamIdx++)
			{
				if (GetNumPlayersOnTeam(TeamIdx) > InNumPlayersPerTeam)
				{
					bTeamSizeError = true;
					UE_LOG(LogBeacon, Warning, TEXT("Beacon has too many players on a team about to be resized."));
				}
			}
		}

		if (!bTeamError && !bTeamSizeError)
		{
			NumTeams = InNumTeams;
			NumPlayersPerTeam = InNumPlayersPerTeam;
			MaxReservations = InNumReservations;

			InitTeamArray();
			bSuccess = true;

			UE_LOG(LogBeacon, Display,
				TEXT("Reconfiguring to team count (%d), team size (%d)"),
				NumTeams,
				NumPlayersPerTeam);
		}
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("Beacon has too many consumed reservations for this reconfiguration, ignoring request."));
	}

	return bSuccess;
}

int32 UB3atZPartyBeaconState::GetMaxAvailableTeamSize() const
{
	int32 MaxFreeSlots = 0;
	// find the largest available free slots within all the teams
	for (int32 TeamIdx = 0; TeamIdx < NumTeams; TeamIdx++)
	{
		MaxFreeSlots = FMath::Max<int32>(MaxFreeSlots, NumPlayersPerTeam - GetNumPlayersOnTeam(TeamIdx));
	}
	return MaxFreeSlots;
}

int32 UB3atZPartyBeaconState::GetNumPlayersOnTeam(int32 TeamIdx) const
{
	int32 Result = 0;
	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FB3atZPartyReservation& Reservation = Reservations[ResIdx];
		if (Reservation.TeamNum == TeamIdx)
		{
			for (int32 PlayerIdx = 0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
			{
				const FB3atZPlayerReservation& PlayerEntry = Reservation.PartyMembers[PlayerIdx];
				// only count valid player net ids
				if (PlayerEntry.UniqueId.IsValid())
				{
					// count party members in each team (includes party leader)
					Result++;
				}
			}
		}
	}
	return Result;
}

int32 UB3atZPartyBeaconState::GetTeamForCurrentPlayer(const FUniqueNetId& PlayerId) const
{
	int32 TeamNum = INDEX_NONE;
	if (PlayerId.IsValid())
	{
		for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
		{
			const FB3atZPartyReservation& Reservation = Reservations[ResIdx];
			for (int32 PlayerIdx = 0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
			{
				// find the player id in the existing list of reservations
				if (*Reservation.PartyMembers[PlayerIdx].UniqueId == PlayerId)
				{
					TeamNum = Reservation.TeamNum;
					break;
				}
			}
		}

		UE_LOG(LogBeacon, Display, TEXT("Assigning player %s to team %d"),
			*PlayerId.ToString(),
			TeamNum);
	}
	else
	{
		UE_LOG(LogBeacon, Display, TEXT("Invalid player when attempting to find team assignment"));
	}

	return TeamNum;
}

int32 UB3atZPartyBeaconState::GetPlayersOnTeam(int32 TeamIndex, TArray<FUniqueNetIdRepl>& TeamMembers) const
{
	TeamMembers.Empty(NumPlayersPerTeam);
	if (TeamIndex < GetNumTeams())
	{
		for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
		{
			const FB3atZPartyReservation& Reservation = Reservations[ResIdx];
			if (Reservation.TeamNum == TeamIndex)
			{
				for (int32 PlayerIdx = 0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
				{
					TeamMembers.Add(Reservation.PartyMembers[PlayerIdx].UniqueId);
				}
			}
		}

		return TeamMembers.Num();
	}
	else
	{
		UE_LOG(LogBeacon, Warning, TEXT("GetPlayersOnTeam: Invalid team index %d"), TeamIndex);
	}
	
	return 0;
}

void UB3atZPartyBeaconState::SetTeamAssignmentMethod(FName NewAssignmentMethod)
{
	TeamAssignmentMethod = NewAssignmentMethod;
}

/**
* Helper for sorting team sizes
*/
struct FTeamBalanceInfo
{
	/** Index of team */
	int32 TeamIdx;
	/** Current size of team */
	int32 TeamSize;

	FTeamBalanceInfo(int32 InTeamIdx, int32 InTeamSize)
		: TeamIdx(InTeamIdx),
		TeamSize(InTeamSize)
	{}
};

/**
 * Sort teams by size (equal teams are randomly mixed)
 */
struct FSortTeamSizeSmallestToLargest
{
	bool operator()(const FTeamBalanceInfo& A, const FTeamBalanceInfo& B) const
	{
		if (A.TeamSize == B.TeamSize)
		{
			return (FMath::Rand() % 2) ? true : false;
		}
		else
		{
			return A.TeamSize < B.TeamSize;
		}
	}
};

int32 UB3atZPartyBeaconState::GetTeamAssignment(const FB3atZPartyReservation& Party)
{
	if (NumTeams > 1)
	{
		TArray<FTeamBalanceInfo> PotentialTeamChoices;
		for (int32 TeamIdx = 0; TeamIdx < NumTeams; TeamIdx++)
		{
			const int32 CurrentPlayersOnTeam = GetNumPlayersOnTeam(TeamIdx);
			if ((CurrentPlayersOnTeam + Party.PartyMembers.Num()) <= NumPlayersPerTeam)
			{
				new (PotentialTeamChoices)FTeamBalanceInfo(TeamIdx, CurrentPlayersOnTeam);
			}
		}

		// Grab one from our list of choices
		if (PotentialTeamChoices.Num() > 0)
		{
			if (TeamAssignmentMethod == ETeamAssignmentMethod::Smallest)
			{
				PotentialTeamChoices.Sort(FSortTeamSizeSmallestToLargest());
				return PotentialTeamChoices[0].TeamIdx;
			}
			else if (TeamAssignmentMethod == ETeamAssignmentMethod::BestFit)
			{
				PotentialTeamChoices.Sort(FSortTeamSizeSmallestToLargest());
				return PotentialTeamChoices[PotentialTeamChoices.Num() - 1].TeamIdx;
			}
			else if (TeamAssignmentMethod == ETeamAssignmentMethod::Random)
			{
				int32 TeamIndex = FMath::Rand() % PotentialTeamChoices.Num();
				return PotentialTeamChoices[TeamIndex].TeamIdx;
			}
		}
		else
		{
			UE_LOG(LogBeacon, Warning, TEXT("UPartyBeaconHost::GetTeamAssignment: couldn't find an open team for party members."));
			return INDEX_NONE;
		}
	}

	return ForceTeamNum;
}

void UB3atZPartyBeaconState::BestFitTeamAssignmentJiggle()
{
	if (TeamAssignmentMethod == ETeamAssignmentMethod::BestFit &&
		NumTeams > 1)
	{
		TArray<FB3atZPartyReservation*> ReservationsToJiggle;
		ReservationsToJiggle.Reserve(Reservations.Num());
		for (FB3atZPartyReservation& Reservation : Reservations)
		{
			// Only want to rejiggle reservations with existing team assignments (new reservations will still stay at -1)
			if (Reservation.TeamNum != -1)
			{
				// Remove existing team assignments so new assignments can be given
				Reservation.TeamNum = -1;
				// Add to list of reservations that need new assignments
				ReservationsToJiggle.Add(&Reservation);
			}
		}
		// Sort so that largest party reservations come first
		ReservationsToJiggle.Sort([](const FB3atZPartyReservation& A, const FB3atZPartyReservation& B)
			{
				return B.PartyMembers.Num() < A.PartyMembers.Num();
			}
		);

		// Re-add these reservations with best fit team assignments
		for (FB3atZPartyReservation* Reservation : ReservationsToJiggle)
		{
			Reservation->TeamNum = GetTeamAssignment(*Reservation);
			if (Reservation->TeamNum == -1)
			{
				UE_LOG(LogBeacon, Warning, TEXT("UPartyBeaconHost::BestFitTeamAssignmentJiggle: could not reassign to a team!"));
			}
		}
	}
}

bool UB3atZPartyBeaconState::AreTeamsAvailable(const FB3atZPartyReservation& ReservationRequest) const
{
	int32 IncomingPartySize = ReservationRequest.PartyMembers.Num();
	for (int32 TeamIdx = 0; TeamIdx < NumTeams; TeamIdx++)
	{
		const int32 CurrentPlayersOnTeam = GetNumPlayersOnTeam(TeamIdx);
		if ((CurrentPlayersOnTeam + IncomingPartySize) <= NumPlayersPerTeam)
		{
			return true;
		}
	}
	return false;
}

bool UB3atZPartyBeaconState::DoesReservationFit(const FB3atZPartyReservation& ReservationRequest) const
{
	int32 IncomingPartySize = ReservationRequest.PartyMembers.Num();
	bool bPartySizeOk = (IncomingPartySize > 0) && (IncomingPartySize <= NumPlayersPerTeam);
	bool bRoomForReservation = (NumConsumedReservations + IncomingPartySize) <= MaxReservations;

	return bPartySizeOk && bRoomForReservation;
}

bool UB3atZPartyBeaconState::AddReservation(const FB3atZPartyReservation& ReservationRequest)
{
	int32 TeamAssignment = GetTeamAssignment(ReservationRequest);
	if (TeamAssignment != INDEX_NONE)
	{
		int32 IncomingPartySize = ReservationRequest.PartyMembers.Num();

		NumConsumedReservations += IncomingPartySize;
		int32 ResIdx = Reservations.Add(ReservationRequest);
		Reservations[ResIdx].TeamNum = TeamAssignment;

		// Possibly shuffle existing teams so that beacon can accommodate biggest open slots
		BestFitTeamAssignmentJiggle();
	}

	return TeamAssignment != INDEX_NONE;
}

bool UB3atZPartyBeaconState::RemoveReservation(const FUniqueNetIdRepl& PartyLeader)
{
	const int32 ExistingReservationIdx = GetExistingReservation(PartyLeader);
	if (ExistingReservationIdx != INDEX_NONE)
	{
		NumConsumedReservations -= Reservations[ExistingReservationIdx].PartyMembers.Num();
		Reservations.RemoveAtSwap(ExistingReservationIdx);

		// Possibly shuffle existing teams so that beacon can accommodate biggest open slots
		BestFitTeamAssignmentJiggle();
		return true;
	}

	return false;
}

void UB3atZPartyBeaconState::RegisterAuthTicket(const FUniqueNetIdRepl& InPartyMemberId, const FString& InAuthTicket)
{
	if (InPartyMemberId.IsValid() && !InAuthTicket.IsEmpty())
	{
		bool bFoundReservation = false;

		for (int32 ResIdx = 0; ResIdx < Reservations.Num() && !bFoundReservation; ResIdx++)
		{
			FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];

			FB3atZPlayerReservation* PlayerRes = ReservationEntry.PartyMembers.FindByPredicate(
				[InPartyMemberId](const FB3atZPlayerReservation& ExistingPlayerRes)
			{
				return InPartyMemberId == ExistingPlayerRes.UniqueId;
			});

			if (PlayerRes)
			{
				UE_LOG(LogBeacon, Display, TEXT("Updating auth ticket for member %s."), *InPartyMemberId.ToString());
				if (!PlayerRes->ValidationStr.IsEmpty() && PlayerRes->ValidationStr != InAuthTicket)
				{
					UE_LOG(LogBeacon, Display, TEXT("Auth ticket changing for member %s."), *InPartyMemberId.ToString());
				}

				PlayerRes->ValidationStr = InAuthTicket;
				bFoundReservation = true;
				break;
			}
		}

		if (!bFoundReservation)
		{
			UE_LOG(LogBeacon, Warning, TEXT("Found no reservation for player %s, while registering auth ticket."), *InPartyMemberId.ToString());
		}
	}
}

void UB3atZPartyBeaconState::UpdatePartyLeader(const FUniqueNetIdRepl& InPartyMemberId, const FUniqueNetIdRepl& NewPartyLeaderId)
{
	if (InPartyMemberId.IsValid() && NewPartyLeaderId.IsValid())
	{
		bool bFoundReservation = false;

		for (int32 ResIdx = 0; ResIdx < Reservations.Num() && !bFoundReservation; ResIdx++)
		{
			FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];

			FB3atZPlayerReservation* PlayerRes = ReservationEntry.PartyMembers.FindByPredicate(
				[InPartyMemberId](const FB3atZPlayerReservation& ExistingPlayerRes)
			{
				return InPartyMemberId == ExistingPlayerRes.UniqueId;
			});

			if (PlayerRes)
			{
				UE_LOG(LogBeacon, Display, TEXT("Updating party leader to %s from member %s."), *NewPartyLeaderId.ToString(), *InPartyMemberId.ToString());
				ReservationEntry.PartyLeader = NewPartyLeaderId;
				bFoundReservation = true;
				break;
			}
		}

		if (!bFoundReservation)
		{
			UE_LOG(LogBeacon, Warning, TEXT("Found no reservation for player %s, while updating party leader."), *InPartyMemberId.ToString());
		}
	}
}

bool UB3atZPartyBeaconState::SwapTeams(const FUniqueNetIdRepl& PartyLeader, const FUniqueNetIdRepl& OtherPartyLeader)
{
	bool bSuccess = false;

	int32 ResIdx = GetExistingReservation(PartyLeader);
	int32 OtherResIdx = GetExistingReservation(OtherPartyLeader);

	if (ResIdx != INDEX_NONE && OtherResIdx != INDEX_NONE)
	{
		FB3atZPartyReservation& PartyRes = Reservations[ResIdx];
		FB3atZPartyReservation& OtherPartyRes = Reservations[OtherResIdx];
		if (PartyRes.TeamNum != OtherPartyRes.TeamNum)
		{
			int32 TeamSize = GetNumPlayersOnTeam(PartyRes.TeamNum);
			int32 OtherTeamSize = GetNumPlayersOnTeam(OtherPartyRes.TeamNum);

			// Will the new teams fit
			bool bValidTeamSizeA = (PartyRes.PartyMembers.Num() + (OtherTeamSize - OtherPartyRes.PartyMembers.Num())) <= NumPlayersPerTeam;
			bool bValidTeamSizeB = (OtherPartyRes.PartyMembers.Num() + (TeamSize - PartyRes.PartyMembers.Num())) <= NumPlayersPerTeam;

			if (bValidTeamSizeA && bValidTeamSizeB)
			{
				Swap(PartyRes.TeamNum, OtherPartyRes.TeamNum);
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

bool UB3atZPartyBeaconState::ChangeTeam(const FUniqueNetIdRepl& PartyLeader, int32 NewTeamNum)
{
	bool bSuccess = false;

	if (NewTeamNum >= 0 && NewTeamNum < NumTeams)
	{
		int32 ResIdx = GetExistingReservation(PartyLeader);
		if (ResIdx != INDEX_NONE)
		{
			FB3atZPartyReservation& PartyRes = Reservations[ResIdx];
			if (PartyRes.TeamNum != NewTeamNum)
			{
				int32 OtherTeamSize = GetNumPlayersOnTeam(NewTeamNum);
				bool bValidTeamSize = (PartyRes.PartyMembers.Num() + OtherTeamSize) <= NumPlayersPerTeam;

				if (bValidTeamSize)
				{
					PartyRes.TeamNum = NewTeamNum;
					bSuccess = true;
				}
			}
		}
	}

	return bSuccess;
}

bool UB3atZPartyBeaconState::RemovePlayer(const FUniqueNetIdRepl& PlayerId)
{
	bool bWasRemoved = false;

	for (int32 ResIdx = 0; ResIdx < Reservations.Num() && !bWasRemoved; ResIdx++)
	{
		FB3atZPartyReservation& Reservation = Reservations[ResIdx];

		if (Reservation.PartyLeader == PlayerId)
		{
			UE_LOG(LogBeacon, Display, TEXT("Party leader has left the party"), *PlayerId.ToString());

			// Maintain existing members of party reservation that lost its leader
			for (int32 PlayerIdx = 0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
			{
				FB3atZPlayerReservation& PlayerEntry = Reservation.PartyMembers[PlayerIdx];
				if (PlayerEntry.UniqueId != Reservation.PartyLeader && PlayerEntry.UniqueId.IsValid())
				{
					// Promote to party leader (for now)
					Reservation.PartyLeader = PlayerEntry.UniqueId;
					break;
				}
			}
		}

		// find the player in an existing reservation slot
		for (int32 PlayerIdx = 0; PlayerIdx < Reservation.PartyMembers.Num(); PlayerIdx++)
		{
			FB3atZPlayerReservation& PlayerEntry = Reservation.PartyMembers[PlayerIdx];
			if (PlayerEntry.UniqueId == PlayerId)
			{
				// player removed
				Reservation.PartyMembers.RemoveAtSwap(PlayerIdx--);
				bWasRemoved = true;

				// free up a consumed entry
				NumConsumedReservations--;
			}
		}

		// remove the entire party reservation slot if no more party members
		if (Reservation.PartyMembers.Num() == 0)
		{
			Reservations.RemoveAtSwap(ResIdx--);
		}
	}

	if (bWasRemoved)
	{
		// Reshuffle existing teams so that beacon can accommodate biggest open slots
		BestFitTeamAssignmentJiggle();
	}

	return bWasRemoved;
}

int32 UB3atZPartyBeaconState::GetExistingReservation(const FUniqueNetIdRepl& PartyLeader) const
{
	int32 Result = INDEX_NONE;
	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];
		if (ReservationEntry.PartyLeader == PartyLeader)
		{
			Result = ResIdx;
			break;
		}
	}

	return Result;
}

int32 UB3atZPartyBeaconState::GetExistingReservationContainingMember(const FUniqueNetIdRepl& PartyMember) const
{
	int32 Result = INDEX_NONE;
	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];
		for (const FB3atZPlayerReservation& PlayerReservation : ReservationEntry.PartyMembers)
		{
			if (PlayerReservation.UniqueId == PartyMember)
			{
				Result = ResIdx;
				break;
			}
		}
	}

	return Result;
}

bool UB3atZPartyBeaconState::PlayerHasReservation(const FUniqueNetId& PlayerId) const
{
	bool bFound = false;

	for (int32 ResIdx = 0; ResIdx < Reservations.Num(); ResIdx++)
	{
		const FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];
		for (int32 PlayerIdx = 0; PlayerIdx < ReservationEntry.PartyMembers.Num(); PlayerIdx++)
		{
			if (*ReservationEntry.PartyMembers[PlayerIdx].UniqueId == PlayerId)
			{
				bFound = true;
				break;
			}
		}
	}

	return bFound;
}

bool UB3atZPartyBeaconState::GetPlayerValidation(const FUniqueNetId& PlayerId, FString& OutValidation) const
{
	bool bFound = false;
	OutValidation = FString();

	for (int32 ResIdx = 0; ResIdx < Reservations.Num() && !bFound; ResIdx++)
	{
		const FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];
		for (int32 PlayerIdx = 0; PlayerIdx < ReservationEntry.PartyMembers.Num(); PlayerIdx++)
		{
			if (*ReservationEntry.PartyMembers[PlayerIdx].UniqueId == PlayerId)
			{
				OutValidation = ReservationEntry.PartyMembers[PlayerIdx].ValidationStr;
				bFound = true;
				break;
			}
		}
	}

	return bFound;
}

bool UB3atZPartyBeaconState::GetPartyLeader(const FUniqueNetIdRepl& InPartyMemberId, FUniqueNetIdRepl& OutPartyLeaderId) const
{
	bool bFoundReservation = false;

	if (InPartyMemberId.IsValid())
	{
		for (int32 ResIdx = 0; ResIdx < Reservations.Num() && !bFoundReservation; ResIdx++)
		{
			const FB3atZPartyReservation& ReservationEntry = Reservations[ResIdx];

			const FB3atZPlayerReservation* PlayerRes = ReservationEntry.PartyMembers.FindByPredicate(
				[InPartyMemberId](const FB3atZPlayerReservation& ExistingPlayerRes)
			{
				return InPartyMemberId == ExistingPlayerRes.UniqueId;
			});

			if (PlayerRes)
			{
				UE_LOG(LogBeacon, Display, TEXT("Found party leader for member %s."), *InPartyMemberId.ToString());
				OutPartyLeaderId = ReservationEntry.PartyLeader;
				bFoundReservation = true;
				break;
			}
		}

		if (!bFoundReservation)
		{
			UE_LOG(LogBeacon, Warning, TEXT("Found no reservation for player %s, while looking for party leader."), *InPartyMemberId.ToString());
		}
	}

	return bFoundReservation;
}

void UB3atZPartyBeaconState::DumpReservations() const
{
	FUniqueNetIdRepl NetId;
	FB3atZPlayerReservation PlayerRes;

	UE_LOG(LogBeacon, Display, TEXT("Session that reservations are for: %s"), *SessionName.ToString());
	UE_LOG(LogBeacon, Display, TEXT("Number of teams: %d"), NumTeams);
	UE_LOG(LogBeacon, Display, TEXT("Number players per team: %d"), NumPlayersPerTeam);
	UE_LOG(LogBeacon, Display, TEXT("Number total reservations: %d"), MaxReservations);
	UE_LOG(LogBeacon, Display, TEXT("Number consumed reservations: %d"), NumConsumedReservations);
	UE_LOG(LogBeacon, Display, TEXT("Number of party reservations: %d"), Reservations.Num());

	// Log each party that has a reservation
	for (int32 PartyIndex = 0; PartyIndex < Reservations.Num(); PartyIndex++)
	{
		NetId = Reservations[PartyIndex].PartyLeader;
		UE_LOG(LogBeacon, Display, TEXT("\t Party leader: %s"), *NetId->ToString());
		UE_LOG(LogBeacon, Display, TEXT("\t Party team: %d"), Reservations[PartyIndex].TeamNum);
		UE_LOG(LogBeacon, Display, TEXT("\t Party size: %d"), Reservations[PartyIndex].PartyMembers.Num());
		// Log each member of the party
		for (int32 MemberIndex = 0; MemberIndex < Reservations[PartyIndex].PartyMembers.Num(); MemberIndex++)
		{
			PlayerRes = Reservations[PartyIndex].PartyMembers[MemberIndex];
			UE_LOG(LogBeacon, Display, TEXT("\t  Party member: %s"), *PlayerRes.UniqueId->ToString());
		}
	}
	UE_LOG(LogBeacon, Display, TEXT(""));
}
