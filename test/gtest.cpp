#include "gtest/gtest.h"
#include "../main.cpp"
#include <string>
#include <memory>

// test fixture 
class ASCII85Test : public ::testing::Test {
protected:
    bool compareVectors(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
        return a == b;
    }

    // input streams from different types
    std::unique_ptr<std::istringstream> createInputStream(const std::string& input) {
        return std::make_unique<std::istringstream>(input);
    }

    std::unique_ptr<std::istringstream> createInputStream(const std::vector<unsigned char>& input) {
        std::string str(input.begin(), input.end());
        return std::make_unique<std::istringstream>(str);
    }

    // helpers to avoid code duplication for encode/decode calls
    template<typename InputType>
    std::vector<unsigned char> callEncodeBuffer(const InputType& input) {
        auto stream = createInputStream(input);
        return encodeBuffer(*stream);
    }
    
    template<typename InputType>
    std::vector<unsigned char> callDecodeBuffer(const InputType& input) {
        auto stream = createInputStream(input);
        return decodeBuffer(*stream);
    }
    
    std::string callEncodeStream(const std::string& input) {
        auto inputStream = createInputStream(input);
        std::ostringstream output;
        encodeStream(*inputStream, output);
        return output.str();
    }

    std::string callDecodeStream(const std::string& input) {
        auto inputStream = createInputStream(input);
        std::ostringstream output;
        decodeStream(*inputStream, output);
        return output.str();
    }
};

/*
    --- checkArguments tests
*/ 

// default encoding
TEST_F(ASCII85Test, CheckArguments_DefaultEncoding) {
    std::vector<std::string_view> args = {"-e"};
    EXPECT_EQ(checkArguments(args), Mode::ENCODE_BUFFER);
}

// decoding
TEST_F(ASCII85Test, CheckArguments_Decoding) {
    std::vector<std::string_view> args = {"-d"};
    EXPECT_EQ(checkArguments(args), Mode::DECODE_BUFFER);
}

// encoding with stream
TEST_F(ASCII85Test, CheckArguments_EncodingStream) {
    std::vector<std::string_view> args = {"-e", "--stream"};
    EXPECT_EQ(checkArguments(args), Mode::ENCODE_STREAM);
}

// decoding with stream
TEST_F(ASCII85Test, CheckArguments_DecodingStream) {
    std::vector<std::string_view> args = {"-d", "--stream"};
    EXPECT_EQ(checkArguments(args), Mode::DECODE_STREAM);
}

// invalid argument
TEST_F(ASCII85Test, CheckArguments_InvalidArgument) {
    std::vector<std::string_view> args = {"--invalid"};
    EXPECT_THROW(checkArguments(args), std::runtime_error);
}

// mixed valid arguments
TEST_F(ASCII85Test, CheckArguments_MixedArguments) {
    std::vector<std::string_view> args = {"-d", "--stream", "-e"};
    EXPECT_EQ(checkArguments(args), Mode::ENCODE_STREAM);
}

/*
    --- encodeBuffer tests
*/

// empty input
TEST_F(ASCII85Test, EncodeBuffer_EmptyInput) {
    auto result = callEncodeBuffer("");
    
    std::vector<unsigned char> expected = {'~', '>'};
    EXPECT_TRUE(compareVectors(result, expected));
}

// single character
TEST_F(ASCII85Test, EncodeBuffer_SingleCharacter) {
    auto result = callEncodeBuffer("A");
    
    // should be encoded to 2 characters + end marker
    EXPECT_GT(result.size(), 2u); // at least encoded chars + ~>
    EXPECT_EQ(result[result.size()-2], '~');
    EXPECT_EQ(result[result.size()-1], '>');
}

// four characters 
TEST_F(ASCII85Test, EncodeBuffer_FourCharacters) {
    auto result = callEncodeBuffer("Test");
    
    // should have 5 encoded characters + end marker
    EXPECT_EQ(result.size(), 7u); 
    EXPECT_EQ(result[5], '~');
    EXPECT_EQ(result[6], '>');
}

/*
    --- encodeStream tests
*/

// empty input
TEST_F(ASCII85Test, EncodeStream_EmptyInput) {
    std::string result = callEncodeStream("");
    EXPECT_EQ(result, "~>");
}

// single character
TEST_F(ASCII85Test, EncodeStream_SingleCharacter) {
    std::string result = callEncodeStream("A");
    EXPECT_GT(result.length(), 2u);
}

// multiple characters
TEST_F(ASCII85Test, EncodeStream_MultipleCharacters) {
    std::string result = callEncodeStream("Hello World");
    EXPECT_GT(result.length(), 2u);
    EXPECT_EQ(result.substr(result.length()-2), "~>");
    
    // check if range is valid
    for (size_t i = 0; i < result.length() - 2; ++i) {
        EXPECT_GE(result[i], 33);
        EXPECT_LE(result[i], 117);
    }
}

/*
    --- decodeBuffer tests
*/

// empty input
TEST_F(ASCII85Test, DecodeBuffer_EmptyInput) {
    auto result = callDecodeBuffer("");
    EXPECT_TRUE(result.empty());
}

// only end marker
TEST_F(ASCII85Test, DecodeBuffer_OnlyEndMarker) {
    auto result = callDecodeBuffer("~>");
    EXPECT_TRUE(result.empty());
}

// invalid character
TEST_F(ASCII85Test, DecodeBuffer_InvalidCharacter) {
    EXPECT_THROW({
        callDecodeBuffer("\x01~>"); // below valid range
    }, std::runtime_error);
}

// character too high
TEST_F(ASCII85Test, DecodeBuffer_CharacterTooHigh) {
    EXPECT_THROW({
        callDecodeBuffer("\x80~>"); // above valid range
    }, std::runtime_error);
}

// with space
TEST_F(ASCII85Test, DecodeBuffer_WithSpace) {
    auto result = callDecodeBuffer("!!!!!\n\t \r~>");
    
    // should ignore space
    EXPECT_EQ(result.size(), 4u); // !!!!! decodes to 4 null bytes
    for (auto byte : result) {
        EXPECT_EQ(byte, 0);
    }
}

// valid coded value
TEST_F(ASCII85Test, DecodeBuffer_ValidCodedValue) {
    auto result = callDecodeBuffer("!!!!!~>");
    
    EXPECT_EQ(result.size(), 4u);
    for (auto byte : result) {
        EXPECT_EQ(byte, 0); // analogically
    }
}

/*
    --- decodeStream tests
*/

// empty input
TEST_F(ASCII85Test, DecodeStream_EmptyInput) {
    std::string result = callDecodeStream("");
    EXPECT_TRUE(result.empty());
}

// only end marker
TEST_F(ASCII85Test, DecodeStream_OnlyEndMarker) {
    std::string result = callDecodeStream("~>");
    EXPECT_TRUE(result.empty());
}

// invalid character
TEST_F(ASCII85Test, DecodeStream_InvalidCharacter) {
    EXPECT_THROW({
        callDecodeStream("\x80~>"); // Use a clearly invalid character
    }, std::runtime_error);
}

// valid coded value with space
TEST_F(ASCII85Test, DecodeStream_ValidCodedValueWithSpace) {
    std::string result = callDecodeStream("!!!!!\n\t \r~>");
    EXPECT_EQ(result.length(), 4u);
    for (char c : result) {
        EXPECT_EQ(c, '\0');
    }
}

/*
    --- encode-decode tests
*/

// simple sentence
TEST_F(ASCII85Test, EncodeDecode_SimpleSentence) {
    std::string original = "Hello, World!";
    
    // encode
    auto encoded = callEncodeBuffer(original);
    
    // decode
    auto decoded = callDecodeBuffer(encoded);
    
    // compare
    std::string decodedStr(decoded.begin(), decoded.end());
    EXPECT_EQ(original, decodedStr);
}

// binary data
TEST_F(ASCII85Test, EncodeDecode_BinaryData) {
    std::vector<unsigned char> original = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    
    auto encoded = callEncodeBuffer(original);
    auto decoded = callDecodeBuffer(encoded);
    
    EXPECT_TRUE(compareVectors(original, decoded));
}

// empty data
TEST_F(ASCII85Test, EncodeDecode_EmptyData) {
    std::string original = "";
    
    auto encoded = callEncodeBuffer(original);
    auto decoded = callDecodeBuffer(encoded);
    
    std::string decodedStr(decoded.begin(), decoded.end());
    EXPECT_EQ(original, decodedStr);
}

/*
    --- stream vs buffer tests
*/

// stream and buffer encoding produce same results
TEST_F(ASCII85Test, StreamVsBuffer_EncodingSameResults) {
    std::string testData = "Encoding same results!";
    
    // buffer encoding
    auto bufferResult = callEncodeBuffer(testData);
    
    // stream encoding
    std::string streamResultStr = callEncodeStream(testData);
    std::vector<unsigned char> streamResult(streamResultStr.begin(), streamResultStr.end());
    
    // compare
    EXPECT_TRUE(compareVectors(bufferResult, streamResult));
}

// stream and buffer decoding produce same results
TEST_F(ASCII85Test, StreamVsBuffer_DecodingConsistency) {
    // Generate valid encoded data first
    std::string testData = "Hello World";
    auto encoded = callEncodeBuffer(testData);
    std::string encodedData(encoded.begin(), encoded.end());
    
    auto bufferResult = callDecodeBuffer(encodedData);
    
    std::string streamResultStr = callDecodeStream(encodedData);
    std::vector<unsigned char> streamResult(streamResultStr.begin(), streamResultStr.end());
    
    EXPECT_TRUE(compareVectors(bufferResult, streamResult));
}