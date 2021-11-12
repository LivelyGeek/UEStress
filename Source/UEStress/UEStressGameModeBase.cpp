// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

#include "UEStressGameModeBase.h"
#include "UEStressPawn.h"

AUEStressGameModeBase::AUEStressGameModeBase()
{
	DefaultPawnClass = AUEStressPawn::StaticClass();
}