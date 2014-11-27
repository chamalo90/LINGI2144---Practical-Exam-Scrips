
l = [ 0x11, 0x45, 0x66, 0x1, 0x88, 0x76, 0x27, 0xfc, 0xab, 0xd6, 0xb4, 0x9a, 0x12, 0x99, 0xe7, 0x00]

str1 = ""
str1b = ""
str2 = ""
str3 = ""
for i in range(0, 8):
    print l[i] ^ l[i+8]
    str1 += " %02x"%l[i]
    str1b += " %02x"%l[i+8]
    str2 += (" (%02x"%l[i]).upper() + " $\oplus$ " + ("%02x)"%l[i+8]).upper()
    str3 += (" %02x"%(l[i] ^ l[i+8])).upper()
    
a = "{"+str1.strip()+str1b+"}"
print a.upper()
print "{"+str2.strip()+"}"
print "{"+str3.strip()+"}"
