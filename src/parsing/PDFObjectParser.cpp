/*
   Source File : PDFObjectParser.cpp


   Copyright 2011 Gal Kahana PDFWriter

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


*/
#include "parsing/PDFObjectParser.h"
#include "BoxingBase.h"
#include "PDFStream.h"
#include "parsing/IPDFParserExtender.h"

#include "Trace.h"
#include "encryption/DecryptionHelper.h"
#include "io/IByteReader.h"
#include "objects/PDFArray.h"
#include "objects/PDFBoolean.h"
#include "objects/PDFDictionary.h"
#include "objects/PDFHexString.h"
#include "objects/PDFIndirectObjectReference.h"
#include "objects/PDFInteger.h"
#include "objects/PDFLiteralString.h"
#include "objects/PDFName.h"
#include "objects/PDFNull.h"
#include "objects/PDFObject.h"

#include "objects/PDFReal.h"
#include "objects/PDFStreamInput.h"
#include "objects/PDFSymbol.h"

#include <sstream>

using namespace charta;

PDFObjectParser::PDFObjectParser()
{
    mParserExtender = nullptr;
    mDecryptionHelper = nullptr;
    mOwnsStream = false;
    mStream = nullptr;
}

PDFObjectParser::~PDFObjectParser()
{
    if (mOwnsStream)
        delete mStream;
}

void PDFObjectParser::SetReadStream(charta::IByteReader *inSourceStream,
                                    IReadPositionProvider *inCurrentPositionProvider, bool inOwnsStream)
{
    if (mOwnsStream)
    {
        delete mStream;
    }

    mStream = inSourceStream;
    mOwnsStream = inOwnsStream;
    mTokenizer.SetReadStream(inSourceStream);
    mCurrentPositionProvider = inCurrentPositionProvider;
    ResetReadState();
}

void PDFObjectParser::ResetReadState()
{
    mTokenBuffer.clear();
    mTokenizer.ResetReadState();
}

void PDFObjectParser::ResetReadState(const PDFParserTokenizer &inExternalTokenizer)
{
    mTokenBuffer.clear();
    mTokenizer.ResetReadState(inExternalTokenizer);
}

static const std::string scR = "R";
static const std::string scStream = "stream";
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseNewObject()
{
    std::string token;

    if (!GetNextToken(token))
        return nullptr;

    // based on the parsed token, and parhaps some more, determine the type of object
    // and how to parse it.

    // Boolean
    if (IsBoolean(token))
        return ParseBoolean(token);
    // Literal String
    if (IsLiteralString(token))
        return ParseLiteralString(token);
    // Hexadecimal String
    if (IsHexadecimalString(token))
        return ParseHexadecimalString(token);
    // NULL
    else if (IsNull(token))
        return std::make_shared<PDFNull>();
    // Name
    else if (IsName(token))
        return ParseName(token);
    // Number (and possibly an indirect reference)
    else if (IsNumber(token))
    {
        auto numberObject = ParseNumber(token);

        // this could be an indirect reference in case this is a positive integer
        // and the next one is also, and then there's an "R" keyword
        if ((numberObject != nullptr) && (numberObject->GetType() == PDFObject::ePDFObjectInteger) &&
            std::static_pointer_cast<charta::PDFInteger>(numberObject)->GetValue() > 0)
        {
            // try parse version
            std::string numberToken;
            if (!GetNextToken(numberToken)) // k. no next token...cant be reference
                return numberObject;

            if (!IsNumber(numberToken)) // k. no number, cant be reference
            {
                SaveTokenToBuffer(numberToken);
                return numberObject;
            }

            auto versionObject = ParseNumber(numberToken);
            bool isReference = false;
            if ((versionObject == nullptr) || (versionObject->GetType() != PDFObject::ePDFObjectInteger) ||
                std::static_pointer_cast<charta::PDFInteger>(versionObject)->GetValue() <
                    0) // k. failure to parse number, or no non-negative, cant be reference
            {
                SaveTokenToBuffer(numberToken);
                return numberObject;
            }

            // try parse R keyword
            std::string keywordToken;
            if (!GetNextToken(keywordToken)) // k. no next token...cant be reference
                return numberObject;

            if (keywordToken != scR) // k. not R...cant be reference
            {
                SaveTokenToBuffer(numberToken);
                SaveTokenToBuffer(keywordToken);
                return numberObject;
            }

            // if passed all these, then this is a reference
            return std::make_shared<PDFIndirectObjectReference>(
                std::static_pointer_cast<charta::PDFInteger>(numberObject)->GetValue(),
                std::static_pointer_cast<charta::PDFInteger>(versionObject)->GetValue());
        }

        return numberObject;
    }
    // Array
    else if (IsArray(token))
        return ParseArray();
    // Dictionary
    else if (IsDictionary(token))
    {
        auto dictObject = std::static_pointer_cast<PDFDictionary>(ParseDictionary());

        if (dictObject != nullptr)
        {
            // could be a stream. will be if the next token is the "stream" keyword
            if (!GetNextToken(token))
                return dictObject;

            if (scStream == token)
            {
                // yes, found a stream. record current position as the position where the stream starts.
                // remove from the current stream position the size of the tokenizer buffer, which is "read", but
                // not used
                return std::make_shared<charta::PDFStreamInput>(
                    dictObject, mCurrentPositionProvider->GetCurrentPosition() - mTokenizer.GetReadBufferSize());
            }
            else
            {
                SaveTokenToBuffer(token);
            }
        }
        return dictObject;
    }
    // Symbol (legitimate keyword or error. determine if error based on semantics)
    else
        return std::make_shared<PDFSymbol>(token);
}

bool PDFObjectParser::GetNextToken(std::string &outToken)
{
    if (!mTokenBuffer.empty())
    {
        outToken = mTokenBuffer.front();
        mTokenBuffer.pop_front();
        return true;
    }

    // skip comments
    BoolAndString tokenizerResult;

    do
    {
        tokenizerResult = mTokenizer.GetNextToken();
        if (tokenizerResult.first && !IsComment(tokenizerResult.second))
        {
            outToken = tokenizerResult.second;
            break;
        }
    } while (tokenizerResult.first);
    return tokenizerResult.first;
}

static const std::string scTrue = "true";
static const std::string scFalse = "false";

bool PDFObjectParser::IsBoolean(const std::string &inToken)
{
    return (scTrue == inToken || scFalse == inToken);
}

std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseBoolean(const std::string &inToken)
{
    return std::make_shared<PDFBoolean>(scTrue == inToken);
}

static const char scLeftParanthesis = '(';
bool PDFObjectParser::IsLiteralString(const std::string &inToken)
{
    return inToken.at(0) == scLeftParanthesis;
}

static const char scRightParanthesis = ')';
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseLiteralString(const std::string &inToken)
{
    std::stringbuf stringBuffer;
    uint8_t buffer;
    std::string::const_iterator it = inToken.begin();
    size_t i = 1;
    ++it; // skip first paranthesis

    // verify that last character is ')'
    if (inToken.at(inToken.size() - 1) != scRightParanthesis)
    {
        TRACE_LOG1("PDFObjectParser::ParseLiteralString, exception in parsing literal string, no closing paranthesis, "
                   "Expression: %s",
                   inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
        return nullptr;
    }

    for (; i < inToken.size() - 1; ++it, ++i)
    {
        if (*it == '\\')
        {
            ++it;
            ++i;
            if ('0' <= *it && *it <= '7')
            {
                buffer = (*it - '0');
                if (i + 1 < inToken.size() && '0' <= *(it + 1) && *(it + 1) <= '7')
                {
                    ++it;
                    ++i;
                    buffer = buffer << 3;
                    buffer += (*it - '0');
                    if (i + 1 < inToken.size() && '0' <= *(it + 1) && *(it + 1) <= '7')
                    {
                        ++it;
                        ++i;
                        buffer = buffer << 3;
                        buffer += (*it - '0');
                    }
                }
            }
            else
            {
                switch (*it)
                {
                case 'n':
                    buffer = '\n';
                    break;
                case 'r':
                    buffer = '\r';
                    break;
                case 't':
                    buffer = '\t';
                    break;
                case 'b':
                    buffer = '\b';
                    break;
                case 'f':
                    buffer = '\f';
                    break;
                case '\\':
                    buffer = '\\';
                    break;
                case '(':
                    buffer = '(';
                    break;
                case ')':
                    buffer = ')';
                    break;
                default:
                    // error!
                    buffer = 0;
                    break;
                }
            }
        }
        else
        {
            buffer = *it;
        }
        stringBuffer.sputn((const char *)&buffer, 1);
    }

    return std::make_shared<PDFLiteralString>(MaybeDecryptString(stringBuffer.str()));
}

std::string PDFObjectParser::MaybeDecryptString(const std::string &inString)
{
    if ((mDecryptionHelper != nullptr) && mDecryptionHelper->IsEncrypted())
    {
        if (mDecryptionHelper->CanDecryptDocument())
            return mDecryptionHelper->DecryptString(inString);

        if (mParserExtender != nullptr)
            return mParserExtender->DecryptString(inString);
        return inString;
    }

    return inString;
}

static const char scLeftAngle = '<';
bool PDFObjectParser::IsHexadecimalString(const std::string &inToken)
{
    // first char should be left angle brackets, and the one next must not (otherwise it's a dictionary start)
    return (inToken.at(0) == scLeftAngle) && (inToken.size() < 2 || inToken.at(1) != scLeftAngle);
}

static const char scRightAngle = '>';
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseHexadecimalString(const std::string &inToken)
{
    // verify that last character is '>'
    if (inToken.at(inToken.size() - 1) != scRightAngle)
    {
        TRACE_LOG1("PDFObjectParser::ParseHexadecimalString, exception in parsing hexadecimal string, no closing "
                   "angle, Expression: %s",
                   inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
        return nullptr;
    }

    return std::make_shared<PDFHexString>(MaybeDecryptString(DecodeHexString(inToken.substr(1, inToken.size() - 2))));
}

std::string PDFObjectParser::DecodeHexString(const std::string &inStringToDecode)
{
    std::stringbuf stringBuffer;
    std::string content = inStringToDecode;

    std::string::const_iterator it = content.begin();
    BoolAndByte buffer(false, 0); // bool part = 'is first char in buffer?', uint8_t part = 'the first hex-decoded char'
    for (; it != content.end(); ++it)
    {
        BoolAndByte parse = GetHexValue(*it);
        if (parse.first)
        {
            if (buffer.first)
            {
                uint8_t hexbyte = (buffer.second << 4) | parse.second;
                buffer.first = false;
                stringBuffer.sputn((const char *)&hexbyte, 1);
            }
            else
            {
                buffer.first = true;
                buffer.second = parse.second;
            }
        }
    }

    // pad with ending 0
    if (buffer.first)
    {
        uint8_t hexbyte = buffer.second << 4;
        stringBuffer.sputn((const char *)&hexbyte, 1);
    }

    // decode utf16 here?
    // Gal: absolutely not! this is plain hex decode. doesn't necesserily mean text. in fact most of the times it
    // doesn't. keep this low level

    return stringBuffer.str();
}

static const std::string scNull = "null";
bool PDFObjectParser::IsNull(const std::string &inToken)
{
    return scNull == inToken;
}

static const char scSlash = '/';
bool PDFObjectParser::IsName(const std::string &inToken)
{
    return inToken.at(0) == scSlash;
}

static const char scSharp = '#';
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseName(const std::string &inToken)
{
    EStatusCode status = charta::eSuccess;
    std::stringbuf stringBuffer;
    BoolAndByte hexResult;
    uint8_t buffer;

    for (auto it = inToken.begin() + 1; it != inToken.end() && charta::eSuccess == status; ++it)
    {
        if (*it == scSharp)
        {
            // hex representation
            ++it;
            if (it == inToken.end())
            {
                TRACE_LOG1("PDFObjectParser::ParseName, exception in parsing hex value for a name token. token = %s",
                           inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
                status = charta::eFailure;
                break;
            }
            hexResult = GetHexValue(*it);
            if (!hexResult.first)
            {
                TRACE_LOG1("PDFObjectParser::ParseName, exception in parsing hex value for a name token. token = %s",
                           inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
                status = charta::eFailure;
                break;
            }
            buffer = (hexResult.second << 4);
            ++it;
            if (it == inToken.end())
            {
                TRACE_LOG1("PDFObjectParser::ParseName, exception in parsing hex value for a name token. token = %s",
                           inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
                status = charta::eFailure;
                break;
            }
            hexResult = GetHexValue(*it);
            if (!hexResult.first)
            {
                TRACE_LOG1("PDFObjectParser::ParseName, exception in parsing hex value for a name token. token = %s",
                           inToken.substr(0, MAX_TRACE_SIZE - 200).c_str());
                status = charta::eFailure;
                break;
            }
            buffer += hexResult.second;
        }
        else
        {
            buffer = *it;
        }
        stringBuffer.sputn((const char *)&buffer, 1);
    }

    if (charta::eSuccess == status)
        return std::make_shared<charta::PDFName>(stringBuffer.str());
    return nullptr;
}

static const char scPlus = '+';
static const char scMinus = '-';
static const char scNine = '9';
static const char scZero = '0';
static const char scDot = '.';
bool PDFObjectParser::IsNumber(const std::string &inToken)
{
    // it's a number if the first char is either a sign or digit, or an initial decimal dot, and the rest is
    // digits, with the exception of a dot which can appear just once.

    if (inToken.at(0) != scPlus && inToken.at(0) != scMinus && inToken.at(0) != scDot &&
        (inToken.at(0) > scNine || inToken.at(0) < scZero))
        return false;

    bool isNumber = true;
    bool dotEncountered = (inToken.at(0) == scDot);
    std::string::const_iterator it = inToken.begin();
    ++it; // verified the first char already

    // only sign is not a number
    if ((inToken.at(0) == scPlus || inToken.at(0) == scMinus) && it == inToken.end())
        return false;

    for (; it != inToken.end() && isNumber; ++it)
    {
        if (*it == scDot)
        {
            if (dotEncountered)
            {
                isNumber = false;
            }
            dotEncountered = true;
        }
        else
            isNumber = (scZero <= *it && *it <= scNine);
    }
    return isNumber;
}

using LongLong = BoxingBaseWithRW<long long>;

std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseNumber(const std::string &inToken)
{
    // once we know this is a number, then parsing is easy. just determine if it's a real or integer, so as to separate
    // classes for better accuracy
    if (inToken.find(scDot) != inToken.npos)
        return std::make_shared<PDFReal>(Double(inToken));
    return std::make_shared<PDFInteger>(LongLong(inToken));
}

static const std::string scLeftSquare = "[";
bool PDFObjectParser::IsArray(const std::string &inToken)
{
    return scLeftSquare == inToken;
}

static const std::string scRightSquare = "]";
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseArray()
{
    auto anArray = std::make_shared<PDFArray>();
    bool arrayEndEncountered = false;
    std::string token;
    EStatusCode status = charta::eSuccess;

    // easy one. just loop till you get to a closing bracket token and recurse
    while (GetNextToken(token) && charta::eSuccess == status)
    {
        arrayEndEncountered = (scRightSquare == token);
        if (arrayEndEncountered)
            break;

        ReturnTokenToBuffer(token);
        auto anObject = ParseNewObject();
        if (!anObject)
        {
            status = charta::eFailure;
            TRACE_LOG1(
                "PDFObjectParser::ParseArray, failure to parse array, failed to parse a member object. token = %s",
                token.substr(0, MAX_TRACE_SIZE - 200).c_str());
        }
        else
        {
            anArray->AppendObject(anObject);
        }
    }

    if (arrayEndEncountered && charta::eSuccess == status)
    {
        return anArray;
    }

    TRACE_LOG1("PDFObjectParser::ParseArray, failure to parse array, didn't find end of array or failure to parse "
               "array member object. token = %s",
               token.substr(0, MAX_TRACE_SIZE - 200).c_str());
    return nullptr;
}

void PDFObjectParser::SaveTokenToBuffer(std::string &inToken)
{
    mTokenBuffer.push_back(inToken);
}

void PDFObjectParser::ReturnTokenToBuffer(std::string &inToken)
{
    mTokenBuffer.push_front(inToken);
}

static const std::string scDoubleLeftAngle = "<<";
bool PDFObjectParser::IsDictionary(const std::string &inToken)
{
    return scDoubleLeftAngle == inToken;
}

static const std::string scDoubleRightAngle = ">>";
std::shared_ptr<charta::PDFObject> PDFObjectParser::ParseDictionary()
{
    auto aDictionary = std::make_shared<PDFDictionary>();
    bool dictionaryEndEncountered = false;
    std::string token;
    EStatusCode status = charta::eSuccess;

    while (GetNextToken(token) && charta::eSuccess == status)
    {
        dictionaryEndEncountered = (scDoubleRightAngle == token);
        if (dictionaryEndEncountered)
            break;

        ReturnTokenToBuffer(token);

        // Parse Key
        auto aKey = ParseNewObject();
        if (!aKey)
        {
            status = charta::eFailure;
            TRACE_LOG1("PDFObjectParser::ParseDictionary, failure to parse key for a dictionary. token = %s",
                       token.substr(0, MAX_TRACE_SIZE - 200).c_str());
            break;
        }

        // Parse Value
        auto aValue = ParseNewObject();
        if (!aValue)
        {
            status = charta::eFailure;
            TRACE_LOG1("PDFObjectParser::ParseDictionary, failure to parse value for a dictionary. token = %s",
                       token.substr(0, MAX_TRACE_SIZE - 200).c_str());
            break;
        }
        auto name = std::static_pointer_cast<charta::PDFName>(aKey);

        // all good. i'm gonna be forgiving here and allow skipping duplicate keys. cause it happens
        if (!aDictionary->Exists(name->GetValue()))
            aDictionary->Insert(name, aValue);
    }

    if (dictionaryEndEncountered && charta::eSuccess == status)
    {
        return aDictionary;
    }

    TRACE_LOG1("PDFObjectParser::ParseDictionary, failure to parse dictionary, didn't find end of array or failure "
               "to parse dictionary member object. token = %s",
               token.substr(0, MAX_TRACE_SIZE - 200).c_str());
    return nullptr;
}

static const char scCommentStart = '%';
bool PDFObjectParser::IsComment(const std::string &inToken)
{
    return inToken.at(0) == scCommentStart;
}

BoolAndByte PDFObjectParser::GetHexValue(uint8_t inValue)
{
    if ('0' <= inValue && inValue <= '9')
        return BoolAndByte(true, inValue - '0');
    if ('A' <= inValue && inValue <= 'F')
        return BoolAndByte(true, inValue - 'A' + 10);
    if ('a' <= inValue && inValue <= 'f')
        return BoolAndByte(true, inValue - 'a' + 10);

    if (isspace(inValue) == 0)
    {
        TRACE_LOG1("PDFObjectParser::GetHexValue, unrecongnized hex value - %c", inValue);
    }
    return BoolAndByte(false, inValue);
}

void PDFObjectParser::SetDecryptionHelper(DecryptionHelper *inDecryptionHelper)
{
    mDecryptionHelper = inDecryptionHelper;
}

void PDFObjectParser::SetParserExtender(charta::IPDFParserExtender *inParserExtender)
{
    mParserExtender = inParserExtender;
}

charta::IByteReader *PDFObjectParser::StartExternalRead()
{
    return mStream;
}

void PDFObjectParser::EndExternalRead()
{
    ResetReadState();
}