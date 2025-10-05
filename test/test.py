import base64
import subprocess

EXEC =  "../ascii85"

# MAIN (DE)CODER SUBPROCESS 
def runAscii85(arguments, input_data: bytes):
    result = subprocess.run (
        [EXEC] + arguments,
        input = input_data,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        check = False
    )
    return result


# ENCODING TESTS
def testEncodeBufferWithoutArguments(): 
    data = b"Hello world!"
    expected = base64.a85encode(data)
    result = runAscii85([], data)

    assert result.returncode == 0, f"ENCODE EXITED WITH {result.returncode}"
    assert result.stdout.strip() == expected, f"ENCODE MISMATCH: {result.stdout} != {expected}"

    print("BUFFER ENCODE WITH EMPTY ARGUMENTS IS OKAY")

def testEncodeBuffer():
    data = b"Hello world!"
    expected = base64.a85encode(data)
    result = runAscii85(["-e"], data)

    assert result.returncode == 0, f"ENCODE EXITED WITH {result.returncode}"
    assert result.stdout.strip() == expected, f"ENCODE MISMATECH: {result.stdout} != {expected}"

    print("BUFFER ENCODE IS OKAY")

def testEncodeStreamWithoutArguments(): 
    data = b"Hello world!"
    expected = base64.a85encode(data)
    result = runAscii85(["--stream"], data)

    assert result.returncode == 0
    assert result.stdout.strip() == expected

    print("STREAM ENCODE IS OKAY")

def testEncodeStream(): 
    data = b"Hello world!"
    expected = base64.a85encode(data)
    result = runAscii85(["-e", "--stream"], data)

    assert result.returncode == 0
    assert result.stdout.strip() == expected

    print("STREAM ENCODE IS OKAY")


# DECODING TESTS
def testDecodeBuffer():
    encoded = base64.a85encode(b"Hello world!")
    result = runAscii85(["-d"], encoded)

    assert result.returncode == 0, f"DECODE EXITED WITH {result.returncode}"
    assert result.stdout == b"Hello world!", "DECODE MISMATCH"

    print("BUFFER DECODE IS OKAY")

def testDecodeStream(): 
    encoded = base64.a85encode(b"Hello world!")
    result = runAscii85(["-d", "--stream"], encoded)

    assert result.returncode == 0
    assert result.stdout == b"Hello world!"

    print("STREAM DECODE IS OKAY")

# INVALID DECODE INPUT 
def testInvalidDecodeInput():
    encoded = b"!!!!xyz~>"  
    result = runAscii85(["-d"], encoded)

    assert result.returncode != 0, "DECODER MUST FALL ON INALID INPUT"

    print("INVALID DECODE INPUT IS BEING PROCESSED CORRECTLY")

if __name__ == "__main__":
    testEncodeBufferWithoutArguments()
    testEncodeBuffer()
    testEncodeStreamWithoutArguments()
    testEncodeStream()

    testDecodeBuffer()
    testDecodeStream()
    testInvalidDecodeInput()

    print("ALL TEST PASSED")