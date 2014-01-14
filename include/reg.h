/*
 * reg.h -- definition of the CCD register file
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef qhy_reg_h
#define qhy_reg_h

#include <string>

namespace qhy {

/**
 * \brief data structure containing all information about the camera
 */
class ccdreg {
public:
	std::string devname;
	unsigned char Gain;
	unsigned char Offset;
	unsigned long Exptime;
	unsigned char HBIN;
	unsigned char VBIN;
	unsigned short LineSize;
	unsigned short VerticalSize;
	unsigned short SKIP_TOP;
	unsigned short SKIP_BOTTOM;
	unsigned short LiveVideo_BeginLine;
	unsigned short AnitInterlace;
	unsigned char MultiFieldBIN;
	unsigned char AMPVOLTAGE;
	unsigned char DownloadSpeed;
	unsigned char TgateMode;
	unsigned char ShortExposure;
	unsigned char VSUB;
	unsigned char CLAMP;
	unsigned char TransferBIT;
	unsigned char TopSkipNull;
	unsigned short TopSkipPix;
	unsigned char MechanicalShutterMode;
	unsigned char DownloadCloseTEC;
	unsigned char SDRAM_MAXSIZE;
	unsigned short ClockADJ;
	unsigned char Trig;
	unsigned char MotorHeating;   //0,1,2
	unsigned char WindowHeater;   //0-15
	unsigned char ADCSEL;
};

/**
 * \brief base class for register file buffers
 */
class register_block {
protected:
	unsigned char	data[64];
public:
	register_block(const ccdreg& reg);
	const unsigned char	*block() const { return data; }
	unsigned char	*block() { return data; }
	void	setpatchnumber(unsigned short patch_number);
};

} // namespace qhy

#endif /* qhy_reg_h */
