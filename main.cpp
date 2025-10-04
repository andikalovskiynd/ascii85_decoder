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
        // take part of 4 bytes
        unsigned char b0 = i < input.size() ? input[i++] : 0;
        unsigned char b1 = i < input.size() ? input[i++] : 0;
        unsigned char b2 = i < input.size() ? input[i++] : 0;
        unsigned char b3 = i < input.size() ? input[i++] : 0;

        int count = std::min<size_t> (4, input.size() - (i - 4));

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

    result.push_back('~');
    result.push_back('>');
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

    out.put('~');
    out.put('>');
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

        char c0 = input[i++] - 33;
        // padding
        char c1 = i < input.size() ? input[i++] - 33 : 84;
        char c2 = i < input.size() ? input[i++] - 33 : 84;
        char c3 = i < input.size() ? input[i++] - 33 : 84;
        char c4 = i < input.size() ? input[i++] - 33 : 84;

        // range check
        for (int idx = 0; idx < 5; ++idx) {
            char c = (idx == 0) ? c0 : (idx == 1) ? c1 : (idx == 2) ? c2 : (idx == 3) ? c3 : c4;
            if (c < 0 || c > 84) {
                char original = c + 33;
                throw std::runtime_error("INVALID ASCII85 CHAR: '" + std::string(1, original) + "' (ASCII " + std::to_string((int)original) + "). Valid range is ! to u (ASCII 33-117)");
            }
        }

        // transform again
        unsigned int decoded = 
        static_cast<unsigned int>(c0) * 85*85*85*85
        + static_cast<unsigned int>(c1) * 85*85*85
        + static_cast<unsigned int>(c2) * 85*85
        + static_cast<unsigned int>(c3) * 85
        + static_cast<unsigned int>(c4);

        result.push_back(static_cast<unsigned char>((decoded >> 24) & 0xFF));
        if (c2 != 84) result.push_back(static_cast<unsigned char>((decoded >> 16) & 0xFF));
        if (c3 != 84) result.push_back(static_cast<unsigned char>((decoded >> 8) & 0xFF));
        if (c4 != 84) result.push_back(static_cast<unsigned char>(decoded & 0xFF));
    }
    
    return result;
}

void decodeStream (std::istream& in, std::ostream& out) {
    while (true) {
        char c[5];
        int count = 0;

        while (count < 5) {
            int ch = in.get();
            if (ch == EOF) break;

            char cc = static_cast<char>(ch);

            // check for insufficient symbols 
            if (cc == ' ' || cc == '\n' || cc == '\r' || cc == '\t') continue;

            // check the end
            if (cc == '~') {
                int next = in.get();
                if (next == '>') return;
                else throw std::runtime_error("INVALID END MARKER");
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
                throw std::runtime_error("INVALID CHARACTER: '" + std::string(1, c[i]) + "' (ASCII " + std::to_string((int)c[i]) + "). Valid range is ! to u (ASCII 33-117)");
            }
        }

        // transform to 32 bit again
        unsigned int decoded = d[0] * 85*85*85*85
        + d[1] * 85*85*85
        + d[2] * 85*85
        + d[3] * 85
        + d[4];

        out.put(static_cast<unsigned char>((decoded >> 24) & 0xFF));
        if (count >= 2) out.put(static_cast<unsigned char>((decoded >> 16) & 0xFF));
        if (count >= 3) out.put(static_cast<unsigned char>((decoded >> 8) & 0xFF));
        if (count >= 4) out.put(static_cast<unsigned char>(decoded & 0xFF));
    }
}

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