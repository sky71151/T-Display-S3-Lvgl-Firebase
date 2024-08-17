/**
 * Created June 12, 2024
 *
 * The MIT License (MIT)
 * Copyright (c) 2024 K. Suwatchai (Mobizt)
 *
 *
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef CORE_JSON_H
#define CORE_JSON_H

#include <Arduino.h>
#include <Client.h>
#include "./Config.h"
#include "./core/Memory.h"
#include "./core/StringUtil.h"
#include "./AsyncResult/Value.h"
#include "./core/Core.h"
#if __has_include(<stdarg.h>)
#include <stdarg.h>
#endif

class JSONUtil
{

private:
public:
    JSONUtil() {}
    ~JSONUtil() {}

    void addObject(String &buf, const String &name, const String &value, bool stringValue, bool last = false)
    {
        if (name.length() == 0)
            return;

        if (buf.length() == 0)
            buf += '{';
        else
            buf += ',';
        if (name[0] != '"')
            buf += '"';
        buf += name;
        if (name[name.length() - 1] != '"')
            buf += '"';
        buf += ':';
        if (stringValue && value[0] != '"')
            buf += '"';
        buf += value;
        if (stringValue && value[value.length() - 1] != '"')
            buf += '"';
        if (last)
            buf += '}';
    }

    void addArray(String &buf, const String &value, bool stringValue, bool last = false)
    {
        if (buf.length() == 0)
            buf += '[';
        else
            buf += ',';
        if (stringValue && value[0] != '"')
            buf += '"';
        buf += value;
        if (stringValue && value[value.length() - 1] != '"')
            buf += '"';
        if (last)
            buf += ']';
    }

    /* convert comma separated tokens into JSON Array and add to JSON object */
    void addTokens(String &buf, const String &name, const String &value, bool last = false)
    {
        StringUtil sut;
        char *p = new char[value.length() + 1];
        memset(p, 0, value.length() + 1);
        strcpy(p, value.c_str());
        char *pp = p;
        char *end = p;
        String tmp;
        if (value.length() == 0)
            tmp += '[';
        while (pp != NULL)
        {
            sut.strsepImpl(&end, ",");
            if (strlen(pp) > 0)
            {
                addArray(tmp, pp, true);
            }
            pp = end;
        }
        tmp += ']';
        addObject(buf, name, tmp, false, last);
        delete p;
    }

    String toString(const String &value)
    {
        String buf;
        buf += '"';
        buf += value;
        buf += '"';
        return buf;
    }
};

class JsonWriter
{

private:
    int prek(object_t &obj, const String &path)
    {
        StringUtil sut;
        char *p = new char[path.length() + 1];
        memset(p, 0, path.length() + 1);
        strcpy(p, path.c_str());
        char *pp = p;
        char *end = p;
        String tmp;
        int i = 0;
        obj = "{";
        while (pp != NULL)
        {
            sut.strsepImpl(&end, "/");
            if (strlen(pp) > 0)
            {
                tmp = pp;
                if (i > 0)
                    obj += '{';
                obj += '"';
                obj += tmp;
                obj += '"';
                obj += ':';
                i++;
            }
            pp = end;
        }

        delete p;
        return i;
    }
    void ek(object_t &obj, int i)
    {
        for (int j = 0; j < i; j++)
            obj += '}';
    }

public:
    JsonWriter() {}
    ~JsonWriter() {}

    void create(object_t &obj, const String &path, bool value) { create(obj, path, boolean_t(value)); }

    template <typename T>
    auto create(object_t &obj, const String &path, T value) -> typename std::enable_if<!std::is_same<T, object_t>::value && !std::is_same<T, string_t>::value && !std::is_same<T, number_t>::value && !std::is_same<T, boolean_t>::value, void>::type
    {
        int i = prek(obj, path);
        if (ValueConverter::v_sring<T>::value)
            obj += "\"";
        obj += value;
        if (ValueConverter::v_sring<T>::value)
            obj += "\"";
        ek(obj, i);
    }

    template <typename T>
    auto create(object_t &obj, const String &path, T value) -> typename std::enable_if<std::is_same<T, object_t>::value || std::is_same<T, string_t>::value || std::is_same<T, number_t>::value || std::is_same<T, boolean_t>::value, void>::type
    {
        int i = prek(obj, path);
        obj += value.c_str();
        ek(obj, i);
    }

    void join(object_t &obj, int nunArgs, ...)
    {
        bool arr = strcmp(obj.c_str(), "[]") == 0;
        obj = "";
        obj += !arr ? '{' : '[';
        va_list ap;
        va_start(ap, nunArgs);
        object_t p = va_arg(ap, object_t);
        if (p)
        {
            if (!arr && (p.c_str()[0] == '{' || p.c_str()[0] == '['))
                obj += p.substring(1, p.length() - 1);
            else
                obj += p;
        }

        for (int i = 2; i <= nunArgs; i++)
        {
            sys_idle();
            obj += ',';
            p = va_arg(ap, object_t);
            if (p)
            {
                if (!arr && (p.c_str()[0] == '{' || p.c_str()[0] == '['))
                    obj += p.substring(1, p.length() - 1);
                else
                    obj += p;
            }
        }
        va_end(ap);
        obj += !arr ? '}' : ']';
    }
};

#endif