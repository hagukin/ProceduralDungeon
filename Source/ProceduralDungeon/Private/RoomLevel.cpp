/*
 * MIT License
 *
 * Copyright (c) 2019-2023 Benoit Pelletier
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RoomLevel.h"
#include "CoreUObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/GameState.h"
#include "GameFramework/Pawn.h"
#include "ProceduralDungeonTypes.h"
#include "Room.h"
#include "RoomData.h"
#include "Door.h"
#include "DungeonGenerator.h"

ARoomLevel::ARoomLevel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bIsInit = false;
	bPendingInit = false;
	Room = nullptr;
	Transform = FTransform::Identity;
}

// Use this for initialization
void ARoomLevel::Init(URoom* _Room)
{
	Room = _Room;
	bIsInit = false;
	bPendingInit = true;

	Transform.SetLocation(Room->Generator()->GetDungeonOffset());
	Transform.SetRotation(Room->Generator()->GetDungeonRotation());

	// Update the room's bounding box for occlusion culling (also the red box drawn in debug)
	UpdateBounds();
}

void ARoomLevel::BeginPlay()
{
	Super::BeginPlay();
}

void ARoomLevel::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Update is called once per frame
void ARoomLevel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsInit && bPendingInit && Room != nullptr)
	{
		SetActorsVisible(Room->IsVisible());

		bPendingInit = false;
		bIsInit = true;
	}

	if (!IsValid(Data))
		return;

#if WITH_EDITOR
	// TODO: Place the debug draw in an editor module of the plugin
	if (URoom::DrawDebug())
	{
		FTransform RoomTransform = (Room != nullptr) ? Room->GetTransform() : FTransform::Identity;

		// Pivot
		DrawDebugSphere(GetWorld(), Transform.TransformPosition(RoomTransform.GetLocation()), 100.0f, 4, FColor::Magenta);

		// Room bounds
		DrawDebugBox(GetWorld(), Transform.TransformPosition(Bounds.Center), Bounds.Extent, Transform.GetRotation(), IsPlayerInside() ? FColor::Green : FColor::Red);

		// Doors
		FVector DoorSize = URoom::DoorSize();
		for (int i = 0; i < Data->GetNbDoor(); i++)
		{
			bool isConnected = Room == nullptr || Room->IsConnected(i);
			ADoor::DrawDebug(GetWorld(), Data->Doors[i].Position, Data->Doors[i].Direction, RoomTransform * Transform, true, isConnected);
		}
	}
#endif
}

void ARoomLevel::UpdateBounds()
{
	if (IsValid(Room))
	{
		Bounds = Room->GetBounds();
	}
	else if (IsValid(Data))
	{
		Bounds = Data->GetBounds();
	}
}

void ARoomLevel::SetActorsVisible(bool Visible)
{
	ULevel* Level = GetLevel();
	if (IsValid(Level))
	{
		for (AActor* Actor : Level->Actors)
		{
			if (IsValid(Actor))
				Actor->SetActorHiddenInGame(!Visible);
		}
	}
}

void ARoomLevel::PostInitProperties()
{
	Super::PostInitProperties();
	UpdateBounds();
}

#if WITH_EDITOR
void ARoomLevel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateBounds();
}
#endif
