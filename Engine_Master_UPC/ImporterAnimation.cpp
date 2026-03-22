#include "Globals.h"
#include "ImporterAnimation.h"

#include "BinaryWriter.h"
#include "BinaryReader.h"
#include "AnimationAsset.h"

Asset* ImporterAnimation::createAssetInstance(const MD5Hash& uid) const
{
	return new AnimationAsset(uid);
}

bool ImporterAnimation::importNative(const std::filesystem::path& path, AnimationAsset* dst)
{
	return false;
}

uint64_t ImporterAnimation::saveTyped(const AnimationAsset* source, uint8_t** outBuffer)
{
	uint64_t size = 0;

	size += sizeof(uint32_t) + source->m_uid.size();
	size += sizeof(uint32_t);                               // channelCount
	
	for(const auto& [nodeName, channel] : source->channels)
	{
		size += sizeof(uint32_t) + nodeName.size();			// channel name string

		size += sizeof(uint32_t);                           // numPositions
		size += channel.numPositions * sizeof(Vector3);     // positions
		size += channel.numPositions * sizeof(float);       // posTimeStamps

		size += sizeof(uint32_t);                           // numRotations
		size += channel.numRotations * sizeof(Quaternion);  // rotations
		size += channel.numRotations * sizeof(float);       // rotTimeStamps
		// Optional scales not implemented yet.
	}

	uint8_t* buffer = new uint8_t[size];
	BinaryWriter writer(buffer);

	writer.string(source->m_uid);
	writer.u32(static_cast<uint32_t>(source->channels.size()));

	for (const auto& [nodeName, channel] : source->channels)
	{
		writer.string(nodeName);

		writer.u32(channel.numPositions);
		writer.bytes(channel.positions.get(), channel.numPositions * sizeof(Vector3));
		writer.bytes(channel.posTimeStamps.get(), channel.numPositions * sizeof(float));

		writer.u32(channel.numRotations);
		writer.bytes(channel.rotations.get(), channel.numRotations * sizeof(Quaternion));
		writer.bytes(channel.rotTimeStamps.get(), channel.numRotations * sizeof(float));
	}

	*outBuffer = buffer;
	return size;
}

void ImporterAnimation::loadTyped(const uint8_t* buffer, AnimationAsset* animation)
{
	BinaryReader reader(buffer);

	animation->m_uid = reader.string();
	const uint32_t channelCount = reader.u32();
	for (uint32_t i = 0; i < channelCount; ++i)
	{
		std::string nodeName = reader.string();
		Channel channel;
		channel.numPositions = reader.u32();
		channel.positions = std::make_unique<Vector3[]>(channel.numPositions);
		reader.bytes(channel.positions.get(), channel.numPositions * sizeof(Vector3));
		channel.posTimeStamps = std::make_unique<float[]>(channel.numPositions);
		reader.bytes(channel.posTimeStamps.get(), channel.numPositions * sizeof(float));
		channel.numRotations = reader.u32();
		channel.rotations = std::make_unique<Quaternion[]>(channel.numRotations);
		reader.bytes(channel.rotations.get(), channel.numRotations * sizeof(Quaternion));
		channel.rotTimeStamps = std::make_unique<float[]>(channel.numRotations);
		reader.bytes(channel.rotTimeStamps.get(), channel.numRotations * sizeof(float));
		animation->channels.emplace(std::move(nodeName), std::move(channel));
	}
}