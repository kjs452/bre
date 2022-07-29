# bre
Business Rule Language. I wrote this library in C to validate orders. The language was byte compiled. Written in 1998.

```
rule r217 using ORDER_HEADER is
	if SPECIAL-CODE-1 in { "28", "29" }
		or SPECIAL-CODE-2 in { "28", "29" }
		or SPECIAL-CODE-3 in { "28", "29" }
		or SPECIAL-CODE-4 in { "28", "29" }
		or SPECIAL-CODE-5 in { "28", "29" }
		or SPECIAL-CODE-6 in { "28", "29" }
		or SPECIAL-CODE-7 in { "28", "29" }
		or SPECIAL-CODE-8 in { "28", "29" }
	then
		    SPECIAL-CODE-1 <> "27"
		and SPECIAL-CODE-2 <> "27"
		and SPECIAL-CODE-3 <> "27"
		and SPECIAL-CODE-4 <> "27"
		and SPECIAL-CODE-5 <> "27"
		and SPECIAL-CODE-6 <> "27"
		and SPECIAL-CODE-7 <> "27"
		and SPECIAL-CODE-8 <> "27"

not ("27" in { SPECIAL-CODE-2, SPECIAL-CODE-3, SPECIAL-CODE-4,
			SPECIAL-CODE-5, SPECIAL-CODE-6, SPECIAL-CODE-7, SPECIAL-CODE-8 })

rule r340 using ORDER_HEADER is
	if STATE-CLASS = "OP" then not (
		(old SPECIAL-CODE-1 <> "SF" and SPECIAL-CODE-1 = "SF") or
		(old SPECIAL-CODE-2 <> "SF" and SPECIAL-CODE-2 = "SF") or
		(old SPECIAL-CODE-3 <> "SF" and SPECIAL-CODE-3 = "SF") or
		(old SPECIAL-CODE-4 <> "SF" and SPECIAL-CODE-4 = "SF") or
		(old SPECIAL-CODE-5 <> "SF" and SPECIAL-CODE-5 = "SF") or
		(old SPECIAL-CODE-6 <> "SF" and SPECIAL-CODE-6 = "SF") or
		(old SPECIAL-CODE-7 <> "SF" and SPECIAL-CODE-7 = "SF") )

rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-1 in {"SP","NO"}
rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-2 in {"SP","NO"}
rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-3 in {"SP","NO"}
rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-4 in {"SP","NO"}
rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-5 in {"SP","NO"}
rule r950 using ORDER_HEADER is
	if TRACE-IC = "T" then COMM-FLAG-6 in {"SP","NO"}

rule r950 using ORDER-HEADER is
	if TRACE-IC = "T" then begin
		for all f in { COMM-FLAG-1, COMM-FLAG-2, COMM-FLAG-3, COMM-FLAG-4, COMM-FLAG-5, COMM-FLAG-6 }
			f in {"SP", "NO"}
	end

rule r839 using ORDER-HEADER is
	if TRADE-IC <> "T" then
		for all R in { S-REP-1, S-REP-2, S-REP-3, S-REP-4,  S-REP-5, S-REP-6 } and
			P in { QU-PERCENT-1, QU-PERCENT-2, QU-PERCENT-3, QU-PERCENT-4, QU-PERCENT-5 }
			if R is set then
				P <= 100


	for all x, y in {a,b,c,d,e}
	begin
		
	end

	for all ORDER_LINE_ITEMS
	begin
		some-boolean-test
	end


	for all TABLE-NAME <statement-block>
	for some TABLE-NAME <statement-block>

	for all IDENTIFIER in list and 


rule r711 using ORDER-HEADER is
	if ORDER-CLASS = "CR" or BILL-STAT[0:1] in {"B", "S"} or STATE-CLASS <> "NE" then
		for some f in { SPECIAL-CODE-1, SPECIAL-CODE-2, SPECIAL-CODE-3, SPECIAL-CODE-4,
					SPECIAL-CODE-5, SPECIAL-CODE-6 }
				if f = "06" then CHANGE-CD <> "00"


-- The second byt of the require report is not changeable by the client
rule r841 using ORDER-HEADER is
	old REQD-REPORT[1:1] = REQD-REPORT[1:1]


rule r104 using ORDER-HEADER is
	if BILL-STAT[0:1] = "S" then SHIP-DATE = null

rule 982 using ORDER-HEADER
	if SHIP-DATE <> null then valid_date(SHIP_DATE)

rule 224 using ORDER-HEADER
	if (ORDER-TYPE = "C2" and ORDER-CLASS = "OR") and
		(SPECIAL-CODE-1 <> "29" or
		 SPECIAL-CODE-2 <> "29" or
		 SPECIAL-CODE-3 <> "29" or
		 SPECIAL-CODE-4 <> "29" or
		 SPECIAL-CODE-5 <> "29" or
		 SPECIAL-CODE-6 <> "29" or
		 SPECIAL-CODE-7 <> "29" or
		 SPECIAL-CODE-8 <> "29")
	then
		SHIP-DATE <> null

	-- not all field 1-8 can contain "29"
	-- Condition applies to at least one.

	-- alternative syntax
	if (ORDER-TYPE = "C2" and ORDER-CLASS = "OR") and
		(for some f in {SPECIAL-CODE-1, SPECIAL-CODE-2, SPECIAL-CODE-3, SPECIAL-CODE-4,
				SPECIAL-CODE-5, SPECIAL-CODE-6, SPECIAL-CODE-7, SPECIAL-CODE-8 }
			f <> "29") then
				SHIP-DATE <> null


rule 599 using ORDER-HEADER is
	if SPCL-PAYMT-TERMS[16:3] <> "    " then
			SPCL-PAYMNT-TERMS[16:1] in { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }
		and	SPCL-PAYMNT-TERMS[17:1] in { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }
		and	SPCL-PAYMNT-TERMS[18:3] in { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }
		and	not SPCL-PAYMNT-TERMS[16:3] = "000";

-- Alternative notation 1
--
-- A constant is introduced to represent a list of items.
-- The substring is compared to null
--
constant digit is { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }

rule 599 using ORDER-HEADER is
	if SPCL-PAYMT-TERMS[16:3] <> null then
			SPCL-PAYMNT-TERMS[16:1] in digit
		and	SPCL-PAYMNT-TERMS[17:1] in digit
		and	SPCL-PAYMNT-TERMS[18:3] in digit
		and	not SPCL-PAYMNT-TERMS[16:3] = "000";

-- Alternative notation 2
--
-- Here we are iterating over the inividual characters in the
-- string.
--
constant digit is { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }

rule 599 using ORDER-HEADER is
	if SPCL-PAYMT-TERMS[16:3] <> null then
		(for all c in SPCL-PAYMT-TERMS[16:3];
			c in digit)
		and
			not SPCL-PAYMNT-TERMS[16:3] = "000";

--- Alternative notation 3
--
-- We are using an assignment to store the substring.
--
constant digit is { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" }

rule 599 using ORDER-HEADER is
	STYPE := SPCL-PAYMT-TERMS[16:3];

	if STYPE <> null then
		(for all c in STYPE is c in digit)
		and not STYPE = "000"


--
-- This checks each SPECIAL-CODE-x field against the FOOBAR table
--
--
rule 025 using ORDER-HEADER is
	for all S in { SPECIAL-CODE-1, SPECIAL-CODE-2, SPECIAL-CODE-3, SPECIAL-CODE-4,
				SPECIAL-CODE-5, SPECIAL-CODE-6, SPECIAL-CODE7 } is
		FooBar["XX", "00", "00", S, "1", "0"] <> null
			or
		FooBar[ORD-SEC-NUM[0:2], "00", "00", S, "1", "0"] <> null
	
```
  
