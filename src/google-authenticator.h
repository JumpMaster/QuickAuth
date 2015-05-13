// Helper program to generate a new secret for use in two-factor
// authentication.
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Adapted for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#define VERIFICATION_CODE_MODULUS (1000*1000) // Six digits
#define BITS_PER_BASE32_CHAR      5           // Base32 expands space by 8/5
	
char *generateCode(const char *key, int timezone_offset)
	__attribute__((visibility("hidden")));