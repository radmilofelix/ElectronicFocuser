EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Transistor_FET:IRLML5203 Q?
U 1 1 61D5D689
P 4550 2500
F 0 "Q?" H 4754 2546 50  0000 L CNN
F 1 "IRLML5203" H 4754 2455 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 4750 2425 50  0001 L CIN
F 3 "https://www.infineon.com/dgdl/irlml5203pbf.pdf?fileId=5546d462533600a40153566868da261d" H 4550 2500 50  0001 L CNN
	1    4550 2500
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 61D5E420
P 4200 2650
F 0 "R?" H 4270 2696 50  0000 L CNN
F 1 "500" H 4270 2605 50  0000 L CNN
F 2 "" V 4130 2650 50  0001 C CNN
F 3 "~" H 4200 2650 50  0001 C CNN
	1    4200 2650
	1    0    0    -1  
$EndComp
$Comp
L pspice:R R?
U 1 1 61D5EE44
P 5300 2600
F 0 "R?" H 5368 2646 50  0000 L CNN
F 1 "300" H 5368 2555 50  0000 L CNN
F 2 "" H 5300 2600 50  0001 C CNN
F 3 "~" H 5300 2600 50  0001 C CNN
	1    5300 2600
	1    0    0    -1  
$EndComp
$EndSCHEMATC
