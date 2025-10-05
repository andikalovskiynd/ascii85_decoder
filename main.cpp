#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include <algorithm>

enum class Mode {
    ENCODE_BUFFER,
    DECODE_BUFFER,
    ENCODE_STREAM,
    DECODE_STREAM
};

Mode checkArguments (const std::vector<std::string_view>& arguments) {
    bool encode = true;
    bool stream = false;

    for (auto argument : arguments) {
        if (argument == "-e") encode = true;
        else if (argument == "-d") encode = false;
        else if (argument == "--stream") stream = true;
        else throw std::runtime_error("UNKNOWN ARGUMENT: " + std::string(argument));
    }

    if (encode && !stream) return Mode::ENCODE_BUFFER;
    if (encode && stream) return Mode::ENCODE_STREAM;
    if (!encode && !stream) return Mode::DECODE_BUFFER;
    return Mode::DECODE_STREAM;
}

// Coders
std::vector<unsigned char> encodeBuffer (std::istream& in) {
    std::vector<unsigned char> input;
    std::vector<unsigned char> result;

    char temp[4096];
    while(in.read(temp, sizeof(temp)) || in.gcount() > 0) {
        input.insert(input.end(), temp, temp + in.gcount());
    }

    size_t i = 0;
    while (i < input.size()) {
        // calculate how many bytes we can read before we actually read them
        int count = std::min<size_t>(4, input.size() - i);
        
        // take part of 4 bytes
        unsigned char b0 = i < input.size() ? input[i++] : 0;
        unsigned char b1 = i < input.size() ? input[i++] : 0;
        unsigned char b2 = i < input.size() ? input[i++] : 0;
        unsigned char b3 = i < input.size() ? input[i++] : 0;

        // get 32 bit number
        unsigned int coded = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;

        // transform to 5 ASCII85 symbols 
        char block[5];
        for (int j = 4; j >= 0; --j) {
            block[j] = static_cast<char>(coded % 85 + 33);
            coded /= 85;
        }

        int outputCount = (count < 4) ? count + 1 : 5;
        result.insert(result.end(), block, block + outputCount);
    }

    return result;
}


void encodeStream (std::istream& in, std::ostream& out) {
    while (true) {
        unsigned char bytes[4] = {0, 0, 0, 0};
        int count = 0;
        
        // read up to 4 bytes
        for (int i = 0; i < 4; ++i) {
            int ch = in.get();
            if (ch == EOF) break;
            bytes[i] = static_cast<unsigned char>(ch);
            count++;
        }
        
        if (count == 0) break; 

        // get 32 bit number again
        unsigned int coded = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

        // transform to 5 ASCII85 symbols 
        char block[5];
        for (int j = 4; j >= 0; --j) {
            block[j] = static_cast<char>(coded % 85 + 33);
            coded /= 85;
        }
        
    int outputCount = (count < 4) ? count + 1 : 5;
    out.write(block, outputCount);
}
}

// Decoders 
std::vector<unsigned char> decodeBuffer(std::istream& in) {
    std::vector<unsigned char> input;
    std::vector<unsigned char> result;

    char temp[4096];
    while(in.read(temp, sizeof(temp)) || in.gcount() > 0) {
        input.insert(input.end(), temp, temp + in.gcount());
    }

    size_t i = 0;
    while (i < input.size()) {
        while (i < input.size() && (input[i] == ' ' || input[i] == '\n' || input[i] == '\r' || input[i] == '\t')) {
            i++;
        }
        if (i >= input.size()) break;

        // check if it is '~>' 
        if (input[i] == '~' && i + 1 < input.size() &&  input[i + 1] == '>') break;

        // check if we have at least 1 character for c0
        if (i >= input.size()) break;
        
        // read up to 5 characters while tracking how many we actually get
        char chars[5] = {84, 84, 84, 84, 84}; // init with padding
        int actualChars = 0;
        
        for (int j = 0; j < 5 && (i + j) < input.size(); j++) {
            if (input[i + j] == '~') break;
            
            // here same as in stream decoder
            if (input[i + j] < 33 || input[i + j] > 117) {
                throw std::runtime_error("INVALID CHARACTER");
            }
            
            chars[j] = input[i + j] - 33;
            actualChars++;
        }
        i += actualChars; 
        
        if (actualChars == 0) break;
        
        char c0 = chars[0];
        char c1 = chars[1]; 
        char c2 = chars[2];
        char c3 = chars[3];
        char c4 = chars[4];

        // range check
        for (int idx = 0; idx < 5; ++idx) {
            char c = (idx == 0) ? c0 : (idx == 1) ? c1 : (idx == 2) ? c2 : (idx == 3) ? c3 : c4;
            if (c < 0 || c > 84) {
                char original = c + 33;
                throw std::runtime_error("INVALID CHARACTER");
            }
        }

        // transform again
        unsigned int decoded = 
        static_cast<unsigned int>(c0) * 85*85*85*85
        + static_cast<unsigned int>(c1) * 85*85*85
        + static_cast<unsigned int>(c2) * 85*85
        + static_cast<unsigned int>(c3) * 85
        + static_cast<unsigned int>(c4);

        int outputBytes = actualChars - 1;
        if (outputBytes > 0) {
            result.push_back(static_cast<unsigned char>((decoded >> 24) & 0xFF));
            if (outputBytes > 1) result.push_back(static_cast<unsigned char>((decoded >> 16) & 0xFF));
            if (outputBytes > 2) result.push_back(static_cast<unsigned char>((decoded >> 8) & 0xFF));
            if (outputBytes > 3) result.push_back(static_cast<unsigned char>(decoded & 0xFF));
        }
    }
    
    return result;
}

void decodeStream (std::istream& in, std::ostream& out) {
    while (true) {
        char c[5];
        int count = 0;
        bool endMarkerFound = false;

        while (count < 5) {
            int ch = in.get();
            if (ch == EOF) break;

            char cc = static_cast<char>(ch);

            // check for insufficient symbols 
            if (cc == ' ' || cc == '\n' || cc == '\r' || cc == '\t') continue;

            // check the end
            if (cc == '~') {
                int next = in.get();
                if (next == '>') {
                    endMarkerFound = true;
                    break;
                }
                else throw std::runtime_error("INVALID END MARKER");
            }

            if (cc < 33 || cc > 117) {
                throw std::runtime_error("INVALID CHARACTER");
            }

            c[count++] = cc;
        }

        if (count == 0) break;

        // padding
        for (int i = count; i < 5; ++i) c[i] = 'u';

        unsigned int d[5];
        for (int i = 0; i < 5; ++i) {
            d[i] = static_cast<unsigned int>(c[i] - 33);
            if (d[i] > 84) {
                throw std::runtime_error("INVALID CHARACTER");
            }
        }

        // transform to 32 bit again
        unsigned int decoded = d[0] * 85*85*85*85
        + d[1] * 85*85*85
        + d[2] * 85*85
        + d[3] * 85
        + d[4];

        int outputBytes = count - 1;
        if (outputBytes > 0) {
            out.put(static_cast<unsigned char>((decoded >> 24) & 0xFF));
            if (outputBytes > 1) out.put(static_cast<unsigned char>((decoded >> 16) & 0xFF));
            if (outputBytes > 2) out.put(static_cast<unsigned char>((decoded >> 8) & 0xFF));
            if (outputBytes > 3) out.put(static_cast<unsigned char>(decoded & 0xFF));
        }
        
        // if we found end marker, exit after processing this part
        if (endMarkerFound) break;
    }
}

#ifndef BUILDING_TESTS
int main(int argc, char* argv[]) {
    try {
        std::vector<std::string_view> arguments(argv + 1, argv + argc);

        if (arguments.empty()) arguments.push_back("-e");

        Mode mode = checkArguments(arguments);

        switch(mode) {
            case Mode::ENCODE_BUFFER: {
                auto result = encodeBuffer(std::cin);
                std::cout.write(reinterpret_cast<const char*>(result.data()), result.size());
                break;
            }

            case Mode::ENCODE_STREAM: {
                encodeStream(std::cin, std::cout);
                break;
            }

            case Mode::DECODE_BUFFER: {
                auto result = decodeBuffer(std::cin);
                std::cout.write(reinterpret_cast<const char*>(result.data()), result.size());
                break;
            }

            case Mode::DECODE_STREAM: {
                decodeStream(std::cin, std::cout);
                break;
            }
        }
        return 0;
    }
    
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1; 
    }
}
#endif