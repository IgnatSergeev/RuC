/*
 *	Copyright 2020 Andrey Terekhov, Egor Anikin
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Errors codes */
typedef enum ERROR
{
	LINKER_CANNOT_OPEN,

	MACRO_NAME_NON,
	MACRO_NAME_FIRST_CHARACTER,
	MACRO_NAME_EXISTS,
} error_t;

/** Warnings codes */
typedef enum WARNING
{
	MACRO_CONSOLE_SEPARATOR,
} warning_t;


/**
 *	Emit an error for some problem
 *
 *	@param	file		File name
 *	@param	str			Code line
 *	@param	line		Line number
 *	@param	symbol		Character number
 *	@param	num			Error code
 */
void macro_error(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const error_t num, ...);

/**
 *	Emit a warning for some problem
 *
 *	@param	file		File name
 *	@param	str			Code line
 *	@param	line		Line number
 *	@param	symbol		Character number
 *	@param	num			Warning code
 */
void macro_warning(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const warning_t num, ...);


/**
 *	Emit an error (embedded version)
 *
 *	@param	file		File name
 *	@param	str			Code line
 *	@param	line		Line number
 *	@param	symbol		Character number
 *	@param	num			Error code
 *	@param	args		Variable list
 */
void macro_verror(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const error_t num, va_list args);

/**
 *	Emit a warning (embedded version)
 *
 *	@param	file		File name
 *	@param	str			Code line
 *	@param	line		Line number
 *	@param	symbol		Character number
 *	@param	num			Warning code
 *	@param	args		Variable list
 */
void macro_vwarning(const char *const file, const char *const str, const size_t line, const size_t symbol
	, const warning_t num, va_list args);


/**
 *	Emit a system error
 *
 *	@param	tag		Message location
 *	@param	num		Error code
 */
void macro_system_error(const char *const tag, const error_t num, ...);

/**
 *	Emit a system warning
 *
 *	@param	tag		Message location
 *	@param	num		Warning code
 */
void macro_system_warning(const char *const tag, const warning_t num, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
