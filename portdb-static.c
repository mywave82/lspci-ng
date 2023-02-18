/* Z370 chipset */
{0x8086, 0x3e1f, 0x1458, 0x5000, "Gigabyte Technologies Co., Ltd", "Z370 AOROS Gaming K3-CF", /* DMIDecode has non-informative labels, one port missing, and one port at the wrong adress */
	{
		{0x00, 0x1c, 0x2, "PCIEX1_1 / slot 1"},
		{0x00, 0x01, 0x0, "PCIEX16 / slot 2"}, /* bridge only visible if a card is inserted, and it offsets all of the bus numbers by one if present */
		{0x00, 0x1c, 0x5, "PCIEX4 / slot 3"},
		{0x00, 0x1c, 0x4, "PCIEX1_2 / slot 4"},
		{0x00, 0x1c, 0x6, "PCIEX1_3 / slot 5"},
		{0x00, 0x1c, 0x7, "PCIEX1_4 / slot 6"}
#if 0
		{0x00, 0x1b, 0x2, "ON-BOARD-1"} /* onboard ASMedia USB controller */
#endif
	}, 6
},

{0x8086, 0x590f, 0x1462, 0x7a68, "MSI", "B250 KRAIT GAMING (MS-7A68)", /* DMIDecode has some of the busaddresses wrong, and non-informative labels */
	{
		{0x00, 0x01, 0x0, "PCI slot 1"},
		{0x00, 0x1c, 0x6, "PCI slot 2/3"},  /* shares the BUS, only use of them at a time */
		{0x00, 0x1b, 0x0, "PCI slot 4"}, 
		{0x00, 0x1c, 0x7, "PCI slot 5/6"} /* shares the BUS, only use of them at a time */
	}, 4
},

{0x8086, 0x3ec2, 0x1043, 0x8694, "ASUSTeK COMPUTER INC.", "PRIME H310M-D", /* partially repaired. All data reported from BIOS is incorrect */
	{
		{0x00, 0x01, 0x00, "PCIEX16"},  // BIOS report 01.01.00, correct is 00.01.0
		{0x00, 0x1c, 0x00, "PCIEX1_1"}, // BIOS report 02.1c.03, correct is 00.1c.0
		// {0x00, 0x1c, 0x07, "INTEGRATED"}, // Not reported by BIOS and it is internal, but it is mapped via PCI root hub
		{0xff, 0x1c, 0x04, "PCIEX1_2"}  // Probably M.2 port, not tested/corrected
	},3
}

#if 0
{foo, bar, test, moo, "ASUSTeK Computer INC.", "M4A785-M", /* SMBIOS version 2.5, does not contain busaddresses */
	{
		{0x00, 0x09, 0x0, "PCIE1X"},
		{0x00, 0x??, 0x?, "PCIE16X",  
		{0x00, 0x0a, 0x0, "PCI1"},
		{0x00, 0x??, 0x?, "PCI2"}
#if 0
		{0x00, 0x01, 0x0, "ON-BOARD-1"} /* onboard Radeon HD 4200 */
		{0x00, 0x14, 0x4, "ON-BOARD-2"} /* onboard RTL8111/8168/8411 */
#endif
	}, 4
},
#endif
