#pragma once

class ISerialisable {
public:
	virtual void serialize(const char** buffer) = 0;

};