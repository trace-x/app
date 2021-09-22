#include "trace_tools.h"

#include "trace_x/trace_x.h"

#include <iostream>

function_t parse_function(const char *string, size_t len)
{
    X_CALL_F;

    X_INFO_F("{}", std::string(string, len));

    function_t result;

    QStringList left_list;

    std::string function;
    std::string args;

    int last_section_begin = -1;

    int last_space = -1;
    int last_left_bracket = -1;
    int arg_begin = -1; //Флаг начала аргументов

    int brackets = 0; // <> counter

    bool first_colon = true;
    bool prev_colon = false;

    // Ньюансы:
    // Имя функции может содержать пробелы (напр, "operator =" )
    // Имя функции может содержать скобки (напр, "operator ()" )
    // Угловая скобка у оператора <<
    // Содержимое аргументов функции можно пропустить

    for(int i = 0; i < int(len); ++i)
    {
        if((arg_begin == -1) && !brackets)
        {
            //Пропускаем символы внутри угловых и круглых скобок

            switch (string[i])
            {
            case ' ':
            {
                QString last_left = QString::fromStdString(std::string(string + last_space + 1, string + i));

                if(last_left.right(8) != "operator")
                {
                    left_list.append(last_left);

                    last_space = i;

                    //Подстрока с префиксами сигнатуры не может оканчиваться на пробел(только на '(')
                    result.namespace_list.clear();
                    last_left_bracket = -1;
                    last_section_begin = -1;
                }

                break;
            }
            case ':':
            {
                if(!prev_colon)
                {
                    std::string section;

                    int begin = (last_section_begin != -1) ? last_section_begin : last_space + 1;

                    if(last_left_bracket > begin)
                    {
                        //prevent template signature
                        section = std::string(string + begin, string + last_left_bracket);
                    }
                    else
                    {
                        section = std::string(string + begin, string + i);
                    }

                    last_section_begin = i;

                    result.namespace_list.append(QString::fromStdString(section));

                    first_colon = false;
                }

                prev_colon = true;

                break;
            }
            case '(':
            {
                if(string[i - 1] != ' ')
                {
                    //Только если предыдущий символ - не пробел, т.к это может быть оператор "()"

                    //Формируем имя функции, т.к оно всегда заканчивается на начале круглой скобки с аргументами
                    int begin = (last_section_begin != -1) ? last_section_begin : last_space + 1;

                    if(last_left_bracket > begin)
                    {
                        //prevent template signature
                        function = std::string(string + begin, string + last_left_bracket);
                    }
                    else
                    {
                        function = std::string(string + begin, string + i);
                    }

                    arg_begin = i + 1;
                }

                break;
            }
            case '<':
            {
                //У лямбда-функций м.б имя класса <lambda_xxx>

                if(string[i - 1] != ':')
                {
                    int begin = (last_section_begin != -1) ? last_section_begin : last_space + 1;

                    QString last_section;

                    if(last_left_bracket > begin)
                    {
                        //prevent template signature
                        last_section = QString::fromStdString(std::string(string + begin, string + last_left_bracket));
                    }
                    else
                    {
                        last_section = QString::fromStdString(std::string(string + begin, string + i));
                    }

                    if(last_section.left(8) != "operator")
                    {
                        last_left_bracket = i;

                        brackets++;
                    }
                }
                else
                {
                    last_section_begin = i;

                    prev_colon = false;
                }

                break;
            }
            default:
            {
                if(prev_colon)
                {
                    last_section_begin = i;
                }

                prev_colon = false;

                break;
            }
            }
        }
        else
        {
            if(arg_begin != -1)
            {
                if(string[i] == ')')
                {
                    args = std::string(string + arg_begin, string + i);
                }
            }
            else
            {
                if(string[i] == '>')
                {
                    brackets--;
                }
                else if(string[i] == '<')
                {
                    brackets++;
                }
            }
        }
    }

    result.function_name = QString::fromStdString(function);
    result.arg = QString::fromStdString(args);
    result.signature = QString::fromLocal8Bit(string, len);

    if(result.function_name.isEmpty())
    {
        result.function_name = string;
    }

    if(!result.namespace_list.isEmpty())
    {
        result.namespace_string = result.namespace_list.join("::");
        result.full_name = result.namespace_string + "::" + result.function_name;
    }
    else
    {
        result.full_name = result.function_name;
    }

    if(left_list.size() > 1)
    {
        result.return_type = left_list.at(left_list.size() - 2);
        result.spec = left_list.last();
    }
    else if(!left_list.isEmpty())
    {
        if(left_list.first() == "__thiscall")
        {
            result.spec = left_list.first();
        }
        else
        {
            result.return_type = left_list.first();
        }
    }

    if(result.spec == "&__thiscall")
    {
        result.spec = "__thiscall";
    }

    return result;
}
