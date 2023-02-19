require "option_parser"
require "./utils.cr"

struct OTP
    @contents : Array(UInt8)

    def initialize (dir : String)
        @contents = File.read(dir).bytes # Read the OTP dump
        if @contents.size != 128 # Panic if the size isn't exactly 128 bytes
            abort "Invalid OTP dump. An OTP should be exactly 128 bytes (Given: #{@contents.size})"
        end
    end

    def read (addr : UInt32): UInt32 # Function to read a BE word from an OTP address
        addr <<= 2 # The OTP is not byte addressed, it's word addressed
        
        (@contents[addr].to_u32 << 24) | (@contents[addr+1].to_u32 << 16) | 
        (@contents[addr+2].to_u32 << 8) | @contents[addr+3].to_u32
    end

    def printRange (name, start : UInt32, finish : UInt32)
        print "#{name}: "
        (start..finish).each do |address|
            print "#{hex_str read(address)}"
        end

        puts
    end
end

directory = "" # OTP file directory

OptionParser.parse do |parser| # Parse CLI args
    parser.banner = "Usage: ./ott <OTP dump directory>"

    parser.on "-f dir", "--file=dir" do |dir|
        directory = dir
        puts directory
    end

    parser.on "-h", "--help" { puts parser }
end

abort "Please gib file" if directory == "" # Abort if no OTP was passed
otp = OTP.new directory

otp.printRange("Boot1 hash", 0, 4)
otp.printRange("Common Key", 5, 8)
puts "NG ID: #{hex_str otp.read(9)}."
otp.printRange("NG Private Key", 0xA, 0x11)
otp.printRange("NAND HMAC", 0x11, 0x15)
otp.printRange("NAND Key", 0x16, 0x19)
otp.printRange("RNG Key", 0x1A, 0x1D)