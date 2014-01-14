/*
 * qhy8pro.cpp -- QHY8PRO class implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <qhy8pro.h>
#include <qhydebug.h>
#include <qhylib.h>
#include <utils.h>

namespace qhy {

/**
 * \brief Create an instance of the Qhy8Pro camera class
 *
 * This constructor must set the essential parameters in the register class
 *
 * \param device
 */
Qhy8Pro::Qhy8Pro(PDevice &device) : CameraOld(device) {
	reg.devname = "QHY8PRO-0";
	reg.Offset = 135;
	reg.Gain = 0;
	reg.SKIP_TOP = 0;
	reg.SKIP_BOTTOM = 0;
	reg.AMPVOLTAGE = 1;
	reg.DownloadSpeed = 1;
	reg.Exptime = 100; // 0.1 s
	reg.LiveVideo_BeginLine = 0;
	reg.AnitInterlace = 1;
	reg.MultiFieldBIN = 0;
	reg.TgateMode = 0;
	reg.ShortExposure = 0;
	reg.VSUB = 0;
	reg.TransferBIT = 0;
	reg.TopSkipNull = 30;
	reg.TopSkipPix = 0;
	reg.MechanicalShutterMode = 0;
	reg.DownloadCloseTEC = 0;
	reg.SDRAM_MAXSIZE = 100;
	reg.ClockADJ = 0x0000;
	reg.ShortExposure = 0;
	size = ImageSize(3328, 2030);
	binningmodes.insert(BinningMode(1, 1));
	binningmodes.insert(BinningMode(2, 2));
	binningmodes.insert(BinningMode(4, 4));
}

/**
 * \brief Set the binning mode mode
 *
 * Setting the binning mode influences quite a few of the variables int
 * the camera register file. The correct values are computed in this
 * method.
 * \param m	the binning mode m
 */
void	Qhy8Pro::mode(const BinningMode& m) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"setting mode for the QHY8PRO camera");
	PCamera::mode(m);
	if (m == BinningMode(1, 1)) {
		reg.HBIN = 1;
		reg.VBIN = 1;
		reg.LineSize = 6656;
		reg.VerticalSize = 1015;
		reg.TopSkipPix = 2300;
		patch_size = 26624;
		return;
	}
	if (m == BinningMode(2, 2)) {
		reg.HBIN = 2;
		reg.VBIN = 1;
		reg.LineSize = 3328;
		reg.VerticalSize = 1015;
		reg.TopSkipPix = 1250;
		patch_size = 26624;
		return;
	}
	if (m == BinningMode(4, 4)) {
		reg.HBIN = 2;
		reg.VBIN = 2;
		reg.LineSize = 3328;
		reg.VerticalSize = 507;
		reg.TopSkipPix = 0;
		patch_size = 3296 * 1024;
		return;
	}
	throw NotSupported("mode not supported");
	// this is redundant, as we should only enter this statement for
	// existing binning modes
}

/**
 * \brief Demultiplexing of image data
 */
void	Qhy8Pro::demux(ImageBuffer& image, const Buffer& buffer) {
	if (_mode == BinningMode(1, 1)) {
		demux11(image, buffer);
		return;
	}
	if (_mode == BinningMode(2, 2)) {
		demux22(image, buffer);
		return;
	}
	if (_mode == BinningMode(4, 4)) {
		demux44(image, buffer);
		return;
	}
}

void	Qhy8Pro::demux11(ImageBuffer& image, const Buffer& buffer) {
	int	PixShift = reg.TopSkipPix;
	int	width = imagesize().first;
	int	height = imagesize().second;
	
    long s,p,m,n;

    s=PixShift*2;
    p=0;
    m=0;
    n=0;

    for (n=0; n < height/2; n++)
    {
        for (m=0;m < width/2;m++)
        {
            image[p+3] = buffer[s+6];  //Gb
            image[p+2] = buffer[s+7];
            image[p+1] = buffer[s+4];
            image[p+0] = buffer[s+5];

            s = s + 8;
            p = p + 4;
        }

        s=s-width*4;
        for (m=0;m< width/2;m++)
        {
            image[p+3-2]  = buffer[s+2];
            image[p+2-2]  = buffer[s+3];
            image[p+1-2]  = buffer[s+0];//Gr
            image[p+0-2]  = buffer[s+1];

            s=s+8;
            p=p+4;
        }
    }
}

void	Qhy8Pro::demux22(ImageBuffer& image, const Buffer& buffer) {
	int	PixShift = reg.TopSkipPix;
	int	width = imagesize().first;
	int	height = imagesize().second;
        long s,k;
        unsigned long binpixel;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"start demultiplexing 2 x 2 binned image");

        s=PixShift*2;
        k=0;

        for (int i=0;i<height;i++)
        {
                for (int j=0;j<width;j++)
                {
                        binpixel = buffer[s]*256 + buffer[s+1] + buffer[s+2]*256 + buffer[s+3];
                        if (binpixel>65535) binpixel=65535;
                        image[k]=LSB((unsigned short)binpixel);
                        k=k+1;
                        image[k]=MSB((unsigned short)binpixel);
                        k=k+1;
                        s=s+4;
                }
        }
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "demultiplexing complete");
}

void	Qhy8Pro::demux44(ImageBuffer& image, const Buffer& buffer) {
	int	PixShift = reg.TopSkipPix;
	int	width = imagesize().first;
	int	height = imagesize().second;

        long s,k;
        unsigned long binpixel;

        s=PixShift*2;
        k=0;

        for (int i=0;i<height;i++){
          for (int j=0;j<width;j++){
                        binpixel=(buffer[s]+buffer[s+2]+buffer[s+4]+buffer[s+6])*256
                                        + buffer[s+1]+buffer[s+3]+buffer[s+5]+buffer[s+7];

                        if (binpixel>65535) binpixel=65535;
                        image[k]=LSB((unsigned short)binpixel);
                        k=k+1;
                        image[k]=MSB((unsigned short)binpixel);
                        k=k+1;

                s=s+8;


          }

        }
}

} // namespace qhy
