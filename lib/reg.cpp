/*
 * reg.cpp -- abstraction of rigester file block 
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <reg.h>
#include <qhydebug.h>
#include <utils.h>
#include <cstring>

namespace qhy {

register_block::register_block(const ccdreg& reg) {
	memset(data, 0, sizeof(data));
	// gain
	data[0] = reg.Gain;

	// offset
	data[1] = reg.Offset;

	// exposure time in milliseconds
	data[2] = (reg.Exptime & 0xff0000) >> 16;
	data[3] = (reg.Exptime & 0x00ff00) >>  8;
	data[4] = (reg.Exptime & 0x0000ff) >>  0;

	// parameters for binning
	data[5] = reg.HBIN;
	data[6] = reg.VBIN;

	// parameters for line length
	data[7] = MSB(reg.LineSize);
	data[8] = LSB(reg.LineSize);

	// vertical image size
	data[9] = MSB(reg.VerticalSize);
	data[10] = LSB(reg.VerticalSize);

	// lines to skip at the top
	data[11] = MSB(reg.SKIP_TOP);
	data[12] = LSB(reg.SKIP_TOP);

	// lines to skip at the bottom
	data[13] = MSB(reg.SKIP_BOTTOM);
	data[14] = LSB(reg.SKIP_BOTTOM);

	// ?
	data[15] = MSB(reg.LiveVideo_BeginLine);
	data[16] = LSB(reg.LiveVideo_BeginLine);

	// Patch number, comes from a different source
	// data[17]
	// data[18]

	// interlace
	data[19] = MSB(reg.AnitInterlace);
	data[20] = LSB(reg.AnitInterlace);
	
	// data[21]

	// multifield binning
	data[22] = reg.MultiFieldBIN;

	// data[23]
	// data[24]
	// data[25]
	// data[26]
	// data[27]
	// data[28]

	data[29] = MSB(reg.ClockADJ);
	data[30] = LSB(reg.ClockADJ);

	// data[31]

	data[32] = reg.AMPVOLTAGE;

	data[33] = reg.DownloadSpeed;
	
	// data[34]

	data[35] = reg.TgateMode;
	data[36] = reg.ShortExposure;
	data[37] = reg.VSUB;
	data[38] = reg.CLAMP;

	// data[39] = 
	// data[40] = 
	// data[41] = 

	data[42] = reg.TransferBIT;

	//data[43] = 
	//data[44] = 
	//data[45] = 

	data[46] = reg.TopSkipNull;
	data[47] = MSB(reg.TopSkipPix);
	data[48] = LSB(reg.TopSkipPix);
	//data[49] = 
	//data[50] = 
	data[51] = reg.MechanicalShutterMode;
	data[52] = reg.DownloadCloseTEC;
	data[53] = ((reg.WindowHeater & ~0xf0) << 4) + (reg.MotorHeating &~ 0xf0);
	//data[54] = 
	//data[55] = 
	//data[56] = 
	data[57] = reg.ADCSEL;
	data[58] = reg.SDRAM_MAXSIZE;
	//data[59] = 
	//data[60] = 
	//data[61] = 
	//data[62] = 
	data[63] = reg.Trig;

}

/**
 * \brief Set the patch number
 *
 * This is the only value that does not solely depend on parameters in
 * the register class.
 */
void	register_block::setpatchnumber(unsigned short patch_number) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "setting patch number %hu",
		patch_number);
	data[17] = MSB(patch_number);
	data[18] = LSB(patch_number);
}


} // namespace qhy
