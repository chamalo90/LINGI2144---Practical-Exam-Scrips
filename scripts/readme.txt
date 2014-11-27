auth.sh:
	usage: ./auth.sh [arg]
		- if "arg" is not given, do nothing after auth
		- if "arg" == "scriptor", launches scriptor after auth
		- else, launches scriptor with "arg" as input script
	
		At runtime, will ask for key (default = "BREAKMEIFYOUCAN!) as string (normal ordering) and challenge (default = 11223344556677)
	APDU:
		FF 00 00 00 04 D4 4A 01 00		-poll the card
		FF 00 00 00 04 D4 42 1A 00		-request challenge
		FF C0 00 00 0E				-get challenge data from reader
		FF 00 00 00 13 D4 42 AF +Challenge	-send challenge back to card
		FF C0 00 00 0E				-get response data from reader (for verification)


read_all_ultralight.sh
	usage: ./read_all_ultralight.sh [type] [last] [first]
		"type" == UC (default), UltralightC with automatic authentication
		"type" == U, Ultralight
		"type" == UCA, UltralightC without auth
		"type" == UCN, UltralightC without auth or polling (used by other scripts or after ./auth.sh)
		
		int last = last sector to read (default read all)
		int first = first sector to read (default 0)
	APDU:
		[FF 00 00 00 04 D4 4A 01 00]		-poll the card (not sent in UCA mode)
		FF 00 00 00 05 D4 40 01 30 + Sector	-request sector
		FF C0 00 00 15				-get data from reader


write_ultralight.sh
	usage: ./write_ultralight.sh [type] [sector] [data]
		"type" same as read
		int sector = sector to write
		data = hex string to write (4 bytes = 8 characters)

	APDU:
		[FF 00 00 00 04 D4 4A 01 00]				-poll the card (not sent in UCA mode)
		FF 00 00 00 15 D4 40 01 A0 + sector + data + O*12	-send write
		FF C0 00 00 15						-get data from read (to make sure write happened well)


change_key.sh
	usage: ./change_key.sh [key]
		key = new key to set (as string, normal ordering)

	APDU:
		none (use write script 4 times)
		
