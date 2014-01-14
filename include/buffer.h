/*
 * buffer.h -- simple buffer allocation management
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef qhy_buffer_h
#define qhy_buffer_h

namespace qhy {

/**
 * \brief Buffer class to ensure proper memory management
 */
class Buffer {
	unsigned long	_length;
public:
	unsigned long	length() const { return _length; }
private:
	unsigned char	*_data;
public:
	unsigned char	*data() const { return _data; }
private:
	Buffer(const Buffer& other);
	Buffer&	operator=(const Buffer& other);
public:
	Buffer(unsigned long l);
	~Buffer();
	const unsigned char&	operator[](unsigned int i) const;
	unsigned char&	operator[](unsigned int i);
};

/**
 * \brief BufferPointer points into a buffer and can be used to add data
 */
class BufferPointer {
	Buffer&	_buffer;
	unsigned long	_offset;
public:
	unsigned long	offset() const { return _offset; }
public:
	BufferPointer(Buffer& buffer);
	void	append(unsigned char *newdata, unsigned long datalength);
	void	append(Buffer& buffer);
	void	append(Buffer& buffer, unsigned long datalength);
};

} // namespace qhy

#endif /* qhy_buffer_h */
