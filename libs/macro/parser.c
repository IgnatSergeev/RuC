/*
 *	Copyright 2021 Andrey Terekhov, Egor Anikin
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

#include "parser.h"
#include <string.h>
#include "error.h"
#include "commenter.h"
#include "keywords.h"
#include "uniprinter.h"
#include "uniscanner.h"


#define CMT_BUFFER_SIZE			MAX_ARG_SIZE + 128
#define UTF8_CHAR_BUFFER_SIZE	8
#define AVERAGE_LINE_SIZE		256

const size_t FST_LINE_INDEX =		1;
const size_t FST_CHARACTER_INDEX =	0;


/**
 *	Checks if сharacter is separator
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
static bool utf8_is_separator(const char32_t symbol)
{
	return  symbol == U' ' || symbol == U'\t';
}

/**
 *	Checks if сharacter is line_breaker
 *
 *	@param	symbol	UTF-8 сharacter
 *
 *	@return	@c 1 on true, @c 0 on false
 */
static bool utf8_is_line_breaker(const char32_t symbol)
{
	return  symbol == U'\r' || symbol == U'\n';
}


/**
 *	Увеличивает значение line и сбрасывает значение position
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_next_line(parser *const prs)
{
	prs->line_position = in_get_position(prs->in);
	prs->line++;
	prs->position = FST_CHARACTER_INDEX;
	strings_clear(&prs->string);
	prs->string = strings_create(AVERAGE_LINE_SIZE);
}

/**
 *	Печатает в prs.out строку кода
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_print(parser *const prs)
{
	const size_t size = strings_size(&prs->string);
	for(size_t i = 0; i < size; i++)
	{
		uni_printf(prs->out, "%s", strings_get(&prs->string, i));
	}

	parser_next_line(prs);
}

/**
 *	Добавляет str в string
 *
 *	@param	prs			Структура парсера
 *	@param	str			Строка
 *
 *	@return	Длина записанной строки
 */
static inline size_t parser_add_string(parser *const prs, const char *const str)
{
	if (str == NULL)
	{
		return 0;
	}

	strings_add(&prs->string, str);
	return strlen(str);
}

/**
 *	Добавляет символ в string и увеличивает значение position
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ
 */
static inline void parser_add_char(parser *const prs, const char32_t cur)
{
	char buffer[UTF8_CHAR_BUFFER_SIZE];
	utf8_to_string(buffer, cur);

	strings_add(&prs->string, buffer);
	prs->position++;
}

/**
 *	Добавляет комментарий в string
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_comment(parser *const prs)
{
	comment cmt = cmt_create(linker_current_path(prs->lk), prs->line);
	char buffer[CMT_BUFFER_SIZE];
	cmt_to_string(&cmt, buffer);
	parser_add_string(prs, buffer);
}

/**
 *	Добавляет макро комментарий в string
 *
 *	@param	prs			Структура парсера
 */
static inline void parser_macro_comment(parser *const prs)
{
	comment cmt = cmt_create_macro(linker_current_path(prs->lk), prs->line, prs->position);
	char buffer[CMT_BUFFER_SIZE];
	cmt_to_string(&cmt, buffer);
	parser_add_string(prs, buffer);
}

/**
 *	Сохраняет считанный код
 *
 *	@param	prs			Структура парсера
 *	@param	str			Строка
 */
static inline size_t parser_add_to_buffer(char *const buffer, const char *const str)
{
	if (str == NULL)
	{
		return 0;
	}

	strcat(buffer, str);
	return strlen(str);
}

/**
 *	Сохраняет считанный символ
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ
 */
static inline void parser_add_char_to_buffer(char *const buffer, const char32_t ch)
{
	utf8_to_string(&buffer[strlen(buffer)], ch);
}


/**
 *	Emit an error from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_macro_error(parser *const prs, const error_t num)
{
	if (parser_is_correct(prs) && !prs->is_recovery_disabled)
	{
		size_t position = in_get_position(prs->in);
		in_set_position(prs->in, prs->line_position);

		char str[256];
		str[0] = '\0';

		char32_t cur = uni_scan_char(prs->in);
		while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			utf8_to_string(&str[strlen(str)], cur);
			cur = uni_scan_char(prs->in);
		}

		prs->was_error = true;
		macro_error(linker_current_path(prs->lk), str, prs->line, prs->position, num);
		in_set_position(prs->in, position);
	}
}

/**
 *	Emit an warning from parser
 *
 *	@param	prs			Parser structure
 *	@param	num			Error code
 */
static void parser_macro_warning(parser *const prs, const warning_t num)
{
	if (parser_is_correct(prs))
	{
		size_t position = in_get_position(prs->in);
		in_set_position(prs->in, prs->line_position);

		char str[256];
		str[0] = '\0';

		char32_t cur = uni_scan_char(prs->in);
		while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
		{
			utf8_to_string(&str[strlen(str)], cur);
			cur = uni_scan_char(prs->in);
		}

		macro_warning(linker_current_path(prs->lk), str, prs->line, prs->position, num);
		in_set_position(prs->in, position);
	}
}


/**
 *	Пропускает строку c текущего символа до '\r', '\n', EOF
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 */
static void parser_skip_line(parser *const prs, char32_t cur)
{
	while (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	uni_unscan_char(prs->in, cur);	// Необходимо, чтобы следующий символ был переносом строки или концом файла
}

/**
 *	Считывает символы до конца строковой константы
 *
 *	@param	prs			Структура парсера
 *	@param	ch			Символ начала строки
 */
static void parser_skip_string(parser *const prs, const char32_t ch)
{
	const size_t position = prs->position;		// Позиция начала строковой константы

	bool was_slash = false;
	parser_add_char(prs, ch);					// Вывод символа начала строковой константы

	char32_t cur = uni_scan_char(prs->in);
	while (cur != (char32_t)EOF)
	{
		if (cur == ch)
		{
			parser_add_char(prs, cur);
			if (was_slash)
			{
				was_slash = cur == U'\\' ? true : false;
			}
			else
			{
				return;							// Строка считана, выход из функции
			}
		}
		else if (utf8_is_line_breaker(cur))		// Ошибка из-за наличия переноса строки
		{
			prs->position = position;
			parser_macro_error(prs, PARSER_STRING_NOT_ENDED);

			if (prs->is_recovery_disabled)		// Добавление '\"' в конец незаконченной строковой константы
			{
				parser_add_char(prs, ch);
			}

			if (cur == U'\r')					// Обработка переноса строки
			{
				uni_scan_char(prs->in);
			}

			parser_add_char(prs, U'\n');
			parser_print(prs);
			return;
		}
		else									// Независимо от корректности строки выводит ее в out
		{
			was_slash = cur == U'\\' ? true : false;
			parser_add_char(prs, cur);
		}

		cur = uni_scan_char(prs->in);
	}

	parser_macro_error(prs, PARSER_UNEXPECTED_EOF);

	if (prs->is_recovery_disabled)				// Добавление "\";" в конец незаконченной строковой константы
	{
		parser_add_char(prs, ch);
		parser_add_char(prs, U';');
	}

	parser_print(prs);
}

/**
 *	Пропускает символы до конца комментария ('\n', '\r' или EOF)
 */
static inline void parser_skip_short_comment(parser *const prs)
{
	parser_skip_line(prs, U'/');
}

/**
 *	Считывает символы до конца длинного комментария
 *
 *	@param	prs			Структура парсера
 */
static void parser_skip_long_comment(parser *const prs)
{
	const size_t line_position = prs->line_position;		// Позиция начала строки с началом комментария
	const size_t line = prs->line;							// Номер строки с началом комментария
	const size_t position = prs->position - 1;				// Позиция начала комментария в строке
	const size_t comment_text_position = in_get_position(prs->in);	// Позиция после символа комментария

	prs->position++;										// '*' был считан снаружи

	bool was_star = false;

	char32_t cur = U'*';
	while (cur != (char32_t)EOF)
	{
		cur = uni_scan_char(prs->in);
		prs->position++;

		switch (cur)
		{
			case U'\r':
				uni_scan_char(prs->in);
			case U'\n':
				parser_print(prs);
					break;

			case U'/':
				if (was_star)
				{
					if (prs->line != line)
					{
						parser_add_char(prs, U'\n');
						parser_comment(prs);
					}
					return;
				}
			default:
				was_star = cur == U'*' ? true : false;
				break;
		}
	}

	prs->line_position = line_position;
	prs->line = line;
	prs->position = position;

	parser_macro_error(prs, PARSER_COMM_NOT_ENDED);

	if (prs->is_recovery_disabled)							// Пропускает начало комментария
	{
		prs->line_position = line_position;
		prs->line = line;
		prs->position = position + 2;
		in_set_position(prs->in, comment_text_position);
	}
}


/**
 *	Проверяет наличие лексем перед директивой препроцессора
 *
 *	@param	prs			Структура парсера
 *	@param	was_lexeme	Флаг, указывающий наличие лексемы
 *
 *	@return	@c 1 если позиция корректна, @c 0 если позиция некорректна 
 */
static inline bool parser_check_kw_position(parser *const prs, const bool was_lexeme)
{
	if (was_lexeme)
	{
		parser_macro_error(prs, PARSER_UNEXPECTED_GRID);
		return 0;
	}

	return -1;
}

/**
 *	Пропускает строку с текущего символа, выводит ошибку, если попался не разделитель или комментарий
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 */
static void parser_find_unexpected_lexeme(parser *const prs, char32_t cur)
{
	while (utf8_is_separator(cur) || cur == U'/')
	{
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'/':
					parser_skip_short_comment(prs);
					return;
				case U'*':
					parser_skip_long_comment(prs);
					break;
				default:
					parser_macro_warning(prs, PARSER_UNEXPECTED_LEXEME);
					parser_skip_line(prs, cur);
					return;
			}
		}

		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	if (!utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_macro_warning(prs, PARSER_UNEXPECTED_LEXEME);
	}

	parser_skip_line(prs, cur);
}

/**
 *	Preprocess included file
 *
 *	@param	prs			Parser structure
 *	@param	path		File path
 *	@param	mode		File search mode
 *						@c '\"' internal
 *						@c '>' external
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_preprocess_file(parser *const prs, const char *const path, const char32_t mode)
{
	parser new_prs = parser_create(prs->lk, prs->stg, prs->out);

	universal_io in = linker_add_header(prs->lk, mode == U'\"'
													? linker_search_internal(prs->lk, path)
													: linker_search_external(prs->lk, path));
	if (!in_is_correct(&in))
	{
		parser_macro_error(prs, PARSER_INCLUDE_INCORRECT_FILENAME);
	}

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	int ret = parser_preprocess(&new_prs, &in);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	return ret;
}

/**
 *	Preprocess buffer
 *
 *	@param	prs			Parser structure
 *	@param	buffer		Code for preprocessing
 *
 *	@return	@c 0 on success, @c -1 on failure
 */
static int parser_preprocess_buffer(parser *const prs, const char *const buffer)
{
	parser_print(prs);

	parser new_prs = parser_create(prs->lk, prs->stg, prs->out);

	universal_io in = io_create();
	in_set_buffer(&in, buffer);

	new_prs.is_recovery_disabled = prs->is_recovery_disabled;
	int ret = parser_preprocess(&new_prs, &in);
	prs->was_error = new_prs.was_error ? new_prs.was_error : prs->was_error;

	in_clear(&in);

	return ret;
}


/**
 *	Считывает путь к файлу и выполняет его обработку
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Символ после директивы
 */
static int parser_include(parser *const prs, char32_t cur)
{
	const size_t position = prs->position;
	prs->position += strlen(storage_last_read(prs->stg)) + 1;	// Учитывается разделитель после директивы

	// Пропуск разделителей и комментариев
	while (utf8_is_separator(cur) || cur == U'/')
	{
		if (cur == U'/')
		{
			char32_t next = uni_scan_char(prs->in);
			switch (next)
			{
				case U'*':
					parser_skip_long_comment(prs);
					break;
				default:
					prs->position = position;
					parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
					parser_skip_line(prs, cur);
					return prs->was_error ? 1 : 0;
			}
		}

		prs->position++;
		cur = uni_scan_char(prs->in);
	}

	if (utf8_is_line_breaker(cur) || cur == (char32_t)EOF || (cur != U'\"' && cur != U'<'))
	{
		prs->position = position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		parser_skip_line(prs, cur);
		return prs->was_error ? 1 : 0;
	}

	// ОБработка пути
	char path[MAX_ARG_SIZE] = "\0";
	const char32_t ch = cur == U'<' ? U'>' : U'\"';
	storage_search(prs->stg, prs->in, &cur);
	prs->position += parser_add_to_buffer(path, storage_last_read(prs->stg));

	while (cur != ch && !utf8_is_line_breaker(cur) && cur != (char32_t)EOF)
	{
		parser_add_char_to_buffer(path, cur);
		prs->position++;

		storage_search(prs->stg, prs->in, &cur);
		prs->position += parser_add_to_buffer(path, storage_last_read(prs->stg));
	}

	prs->position++;
	if (cur != ch)
	{
		prs->position = position;
		parser_macro_error(prs, PARSER_INCLUDE_NEED_FILENAME);
		parser_skip_line(prs, cur);
		return prs->was_error ? 1 : 0;
	}

	// Пропуск символов за путем
	prs->position = temp;
	parser_find_unexpected_lexeme(prs, uni_scan_char(prs->in));

	// Парсинг подключенного файла
	size_t temp = prs->position;
	prs->position = position;
	int ret = parser_preprocess_file(prs, path, ch);

	return ret;
}


/**
 *	Разбирает код
 *
 *	@param	prs			Структура парсера
 *	@param	cur			Текущий символ
 *	@param	mode		Режим работы функции
 */
static void parser_preprocess_code(parser *const prs, char32_t cur, const keyword_t mode)
{
	size_t index = 0;
	bool was_star = false;
	bool was_lexeme = false;
	while (cur != (char32_t)EOF)
	{
		index = storage_search(prs->stg, prs->in, &cur);
		switch (index)
		{
			case KW_INCLUDE:
				parser_check_kw_position(prs, was_lexeme);	// Проверка на наличие лексем перед директивой
				parser_include(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
		
			case KW_DEFINE:
			case KW_SET:
			case KW_UNDEF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_MACRO:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ENDM:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_error(prs, PARSER_UNEXPECTED_ENDM);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_IFDEF:
			case KW_IFNDEF:
			case KW_IF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ELIF:
			case KW_ELSE:
			case KW_ENDIF:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_error(prs, PARSER_UNEXPECTED_ENDIF);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;

			case KW_EVAL:

			case KW_WHILE:
				parser_check_kw_position(prs, was_lexeme);
				parser_macro_warning(prs, 100);
				parser_skip_line(prs, cur);
				was_lexeme = false;
				was_star = false;
				break;
			case KW_ENDW:
				parser_check_kw_position(prs, was_lexeme);
				if (mode != KW_WHILE)
				{
					parser_macro_error(prs, PARSER_UNEXPECTED_ENDW);
					parser_skip_line(prs, cur);
				}
				was_lexeme = false;
				was_star = false;
				return;

			default:
				if (!utf8_is_separator(cur) || storage_last_read(prs->stg) != NULL)	// Перед '#' могут быть только разделители
				{
					was_lexeme = true;
				}

				if (storage_last_read(prs->stg) != NULL)
				{
					was_star = false;

					if (storage_last_read(prs->stg)[0] == '#')
					{
						if (parser_check_kw_position(prs, was_lexeme))	// Перед '#' есть лексемы -> '#' не на месте
																		// Перед '#' нет лексем   -> неправильная директива
						{
							parser_macro_error(prs, PARSER_UNIDETIFIED_KEYWORD);
						}
						parser_skip_line(prs, cur);
						break;
					}

					if (index != SIZE_MAX)
					{
						// Макроподстановка
						parser_preprocess_buffer(prs, storage_get_by_index(prs->stg, index));
					}
					else
					{
						prs->position += parser_add_string(prs, storage_last_read(prs->stg));
					}
				}

				switch (cur)
				{
					case U'#':
						was_star = false;
						parser_macro_error(prs, PARSER_UNEXPECTED_GRID);
						parser_skip_line(prs, cur);
						break;

					case U'\'':
						was_star = false;
						parser_skip_string(prs, U'\'');
						break;
					case U'\"':
						was_star = false;
						parser_skip_string(prs, U'\"');
						break;

					case U'\r':
						cur = uni_scan_char(prs->in);
					case U'\n':
						was_star = false;
						was_lexeme = false;
						parser_add_char(prs, U'\n');
						parser_print(prs);
						break;
					case (char32_t)EOF:
						parser_print(prs);
						break;

					case U'/':
					{
						char32_t next = uni_scan_char(prs->in);
						if (next == U'/')
						{
							was_star = false;
							parser_skip_short_comment(prs);
						}
						else if (next == U'*')
						{
							was_star = false;
							prs->position++;
							parser_skip_long_comment(prs);
						}
						else
						{
							uni_unscan_char(prs->in, next);
							if (was_star)
							{
								was_star = false;
								strings_remove(&prs->string);	// '*' был записан в буффер
								prs->position--;
								parser_macro_warning(prs, PARSER_UNEXPECTED_COMM_END);
								prs->position += 2;	// необходимо пропустить
							}
							else
							{
								parser_add_char(prs, cur);
							}
						}
					}
					break;
					case U'*':
						was_star = true;
						parser_add_char(prs, cur);
						break;

					default:
						was_star = false;
						parser_add_char(prs, cur);
				}
		}
	}
}


/*
 *	 __     __   __     ______   ______     ______     ______   ______     ______     ______
 *	/\ \   /\ "-.\ \   /\__  _\ /\  ___\   /\  == \   /\  ___\ /\  __ \   /\  ___\   /\  ___\
 *	\ \ \  \ \ \-.  \  \/_/\ \/ \ \  __\   \ \  __<   \ \  __\ \ \  __ \  \ \ \____  \ \  __\
 *	 \ \_\  \ \_\\"\_\    \ \_\  \ \_____\  \ \_\ \_\  \ \_\    \ \_\ \_\  \ \_____\  \ \_____\
 *	  \/_/   \/_/ \/_/     \/_/   \/_____/   \/_/ /_/   \/_/     \/_/\/_/   \/_____/   \/_____/
 */


parser parser_create(linker *const lk, storage *const stg, universal_io *const out)
{
	parser prs; 
	if (!linker_is_correct(lk) || !out_is_correct(out) || !storage_is_correct(stg))
	{
		prs.lk = NULL;
		return prs;
	}

	prs.lk = lk;
	prs.stg = stg;
	
	prs.in = NULL;
	prs.out = out;

	prs.line_position = 0;
	prs.line = FST_LINE_INDEX;
	prs.position = FST_CHARACTER_INDEX;
	prs.string = strings_create(AVERAGE_LINE_SIZE);

	prs.is_recovery_disabled = false;
	prs.was_error = false;

	return prs;
} 


int parser_preprocess(parser *const prs, universal_io *const in)
{
	if (!parser_is_correct(prs)|| !in_is_correct(in))
	{
		return -1;
	}

	prs->in = in;
	parser_comment(prs);

	char32_t cur = U'\0';
	parser_preprocess_code(prs, cur, 0);

	return !prs->was_error ? 0 : -1;
}


int parser_disable_recovery(parser *const prs)
{
	if (!parser_is_correct(prs))
	{
		return -1;
	}

	prs->is_recovery_disabled = true;
	return 0;
}


bool parser_is_correct(const parser *const prs)
{
	return prs != NULL && linker_is_correct(prs->lk) && storage_is_correct(prs->stg) && out_is_correct(prs->out);
}


int parser_clear(parser *const prs)
{
	return prs != NULL && linker_clear(prs->lk) && storage_clear(prs->stg) &&  strings_clear(&prs->string);
}
