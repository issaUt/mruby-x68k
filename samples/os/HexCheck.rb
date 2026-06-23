# coding: Windows-31J

fn_hex = ""
fn_bin = "OUT.BIN"
quiet = false
arg_index = 0
progress_step = 0x100
next_progress = 0

ARGV.each do | arg |
    if (arg == "-q") || (arg == "--quiet") then
        quiet = true
        next
    end

    case arg_index
    when 0
        fn_hex = arg.dup
    when 1
        fn_bin = arg.dup
    end
    arg_index += 1
end

printf("Input : %s\n", fn_hex)
printf("Output: %s\n", fn_bin)
printf("Quiet : on\n") if quiet

unless File.exist?(fn_hex) then
    printf("'%s' :ファイルがみつかりません", fn_hex)
    exit
end

fp = open(fn_hex, "r")
start_adr = 0

checkAry = [0, 0, 0, 0, 0, 0, 0, 0, 0]
aryData = []

while line = fp.gets()
    line.chomp!
    if line =~ /^START/ then
        tmp = line.split(',')
        if tmp.length == 2 then
            adr_str = "0x" + tmp[1].strip
            start_adr = adr_str.to_i(16)
            next_progress = start_adr
            print "StartAdr: ", start_adr.to_s(16), "\n"
        end
        next
    elsif line =~ /^[A-F,S,0-9]/ then
        tmp = line.split(' ')
        aryValue = []
        adr_str = 0
        check = 0
        tmp.each_with_index do | val, i |
            case i
            when 0
                adr_str = "0X" + val
                currentStrAdr = adr_str.to_i(16)
            when 1..8
                aryValue[i-1] = ("0X" + val).to_i(16)
            when 9
                if val[0] == ':' then
                    aryValue[i-1] = ("0x" + val[1..2]).to_i(16)
                else
                    aryValue[i-1] = -1
                end
            end
        end

        if line =~ /^[A-F,0-9]/ then
            if quiet && start_adr >= next_progress then
                printf("ADDR: %04X\n", start_adr)
                next_progress += progress_step
            end
            printf("%04X ", start_adr) unless quiet

            check = 0
            aryValue.each_with_index do | val, i |
                checkAry[i] += val
                if i < 8 then
                    printf("%02X ", val) unless quiet
                    check = check + val
                    aryData.push(val)
                else
                    printf(":%02X", val) unless quiet
                end
            end

            check = check & 0xFF
            if check == aryValue[8] then
                # OK
            else
                # Error
                if quiet then
                    print "Error at ", start_adr.to_s(16).upcase, ": Check = ", check.to_s(16).upcase
                else
                    print " Error: Check = ", check.to_s(16).upcase
                end
                print "\n"
            end
            start_adr += 8
            print "\n" unless quiet

        elsif line =~ /^SUM/ then
            # check V
            print "SUM: "
            aryValue.each_with_index do | val, i |
                if i < 8 then
                    printf("%02X ", val)
                else
                    printf(":%02X", val)
                end
            end
            print "\n"

            print "CHK:"
            aryValue.each_with_index do | val, i |
                check = checkAry[i] & 0xFF
                if check == val then
                    print " .."
                else
                    printf("*%02X", check)
                end
            end
            print "\n"

            for i in 0..checkAry.length do
                checkAry[i] = 0
            end
            print "\n"
        end
    end
end

fp.close

# bin out
fp = open(fn_bin, "wb")
fp.write(aryData.pack('c*'))
fp.close

exit

