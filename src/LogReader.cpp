#include <iostream>
#include <fstream>
#include <pocolog_cpp/Write.hpp>
#include "LogReader.hpp"

namespace rock_replay_cpp
{

void LogReader::exportStream(
    const std::string &filename,
    const std::string &stream_name,
    int start_index,
    int final_index,
    export_stream_fcn_t fcn,
    void *data)
{
  pocolog_cpp::StreamDescription desc;
  loadStreamDescription(stream_name, desc);

  std::vector<pocolog_cpp::StreamMetadata> metadata = getMetadata(desc);

  std::ofstream os(filename.c_str(), std::ofstream::binary | std::ofstream::out);
  pocolog_cpp::Output output(os);

  output.writeStreamDeclaration(
      0,
      desc.getType(),
      desc.getName(),
      desc.getTypeName(),
      desc.getTypeDescription(),
      metadata);

  pocolog_cpp::FileStream fileStream;

  fileStream.open(
      desc.getFileName().c_str(),
      std::ifstream::binary | std::ifstream::in);

  if (!fileStream.good())
    throw std::runtime_error("Error, could not open logfile for stream " + desc.getName());

  pocolog_cpp::InputDataStream *data_stream = stream(stream_name).input_data_stream();

  for (int sampleNr = start_index; sampleNr < final_index; sampleNr++)
  {
    std::streampos samplePos = data_stream->getFileIndex().getSamplePos(sampleNr);
    std::streampos sampleHeaderPos = samplePos;
    sampleHeaderPos -= sizeof(pocolog_cpp::SampleHeaderData);

    pocolog_cpp::SampleHeaderData header;

    fileStream.seekg(sampleHeaderPos);
    fileStream.read((char *)&header, sizeof(pocolog_cpp::SampleHeaderData));
    if (!fileStream.good())
      throw std::runtime_error("Could not load sample header");

    std::vector<uint8_t> buffer;
    buffer.resize(header.data_size);

    fileStream.seekg(samplePos);
    fileStream.read((char *)buffer.data(), header.data_size);

    if (!fileStream.good())
      throw std::runtime_error("Could not load sample data");

    base::Time realtime = base::Time::fromSeconds(header.realtime_tv_sec, header.realtime_tv_usec);
    base::Time logical = base::Time::fromSeconds(header.timestamp_tv_sec, header.timestamp_tv_usec);

    if (data_stream->getSampleData(buffer, sampleNr))
    {
      output.writeSample(
          0,
          realtime,
          logical,
          (void *)buffer.data(),
          buffer.size());

      if (fcn != NULL)
        if (!fcn(sampleNr, data))
          break;
    }
  }
}

void LogReader::loadStreamDescription(
    const std::string &stream_name,
    pocolog_cpp::StreamDescription &desc)
{
  const std::vector<pocolog_cpp::StreamDescription> descriptions = log_file_.getStreamDescriptions();
  for (std::vector<pocolog_cpp::StreamDescription>::const_iterator it = descriptions.begin(); it != descriptions.end(); it++)
  {
    switch (it->getType())
    {
    case pocolog_cpp::DataStreamType:
      if (it->getName() == stream_name)
      {
        desc = *it;
        return;
      }
      break;
    default:
      std::cout << "Ignoring stream " << it->getName() << std::endl;
      break;
    }
  }
}

std::vector<pocolog_cpp::StreamMetadata>
LogReader::getMetadata(pocolog_cpp::StreamDescription desc)
{
  std::map<std::string, std::string> metadata_map = desc.getMetadataMap();
  std::vector<pocolog_cpp::StreamMetadata> metadata_list;

  for (std::map<std::string, std::string>::iterator it = metadata_map.begin();
       it != metadata_map.end(); it++)
  {
    pocolog_cpp::StreamMetadata data;
    data.key = it->first;
    data.value = it->second;
    metadata_list.push_back(data);
  }
  return metadata_list;
}

std::vector<pocolog_cpp::StreamDescription>
LogReader::getDescriptions()
{
  std::vector<pocolog_cpp::StreamDescription> result;
  const std::vector<pocolog_cpp::StreamDescription> descriptions = log_file_.getStreamDescriptions();
  for (std::vector<pocolog_cpp::StreamDescription>::const_iterator it = descriptions.begin(); it != descriptions.end(); it++)
  {
    switch (it->getType())
    {
    case pocolog_cpp::DataStreamType:
      result.push_back(*it);
      break;
    default:
      std::cout << "Ignoring stream " << it->getName() << std::endl;
      break;
    }
  }
  return result;
}

} // namespace rock_replay_cpp