#ifndef LogReader_hpp
#define LogReader_hpp

#include <cstring>
#include <string>
#include <QtGui/QWidget>
#include <pocolog_cpp/LogFile.hpp>
#include <pocolog_cpp/InputDataStream.hpp>

namespace rock_replay_cpp
{

typedef bool (*export_stream_fcn_t)(int, void*);

class LogStream
{
public:
  template <typename T>
  bool read_sample(T &sample, size_t sample_index)
  {
    memset(&sample, 0, sizeof(T));
    if (sample_index < total_samples())
    {
      data_stream_->getSample<T>(sample, sample_index);
      return true;
    }
    return false;
  }

  template <typename T>
  bool next(T &sample)
  {
    return read_sample<T>(sample, current_sample_index_++);
  }

  void reset()
  {
    current_sample_index_ = 0;
  }

  size_t total_samples()
  {
    return data_stream_->getSize();
  }

  size_t current_sample_index()
  {
    return current_sample_index_;
  }

  void set_current_sample_index(size_t index)
  {
    current_sample_index_ = index;
  }

  LogStream()
      : data_stream_(NULL)
  {
  }

  pocolog_cpp::InputDataStream *input_data_stream() const
  {
    return data_stream_;
  }

private:
  LogStream(pocolog_cpp::InputDataStream *data_stream)
      : data_stream_(data_stream), current_sample_index_(0)
  {
  }

  pocolog_cpp::InputDataStream *data_stream_;
  size_t current_sample_index_;

  friend class LogReader;
}; // namespace classLogStream

class LogReader
{
public:
  LogReader(const std::string &input_file_path)
      : log_file_(input_file_path)
  {
  }

  ~LogReader()
  {
  }

  LogStream stream(const std::string &stream_name)
  {
    try
    {
      return LogStream(dynamic_cast<pocolog_cpp::InputDataStream *>(&log_file_.getStream(stream_name)));
    }
    catch (...)
    {
      throw;
    }
    return LogStream();
  }

  const pocolog_cpp::LogFile &log_file() const
  {
    return log_file_;
  }

  void exportStream(const std::string &filename,
                     const std::string &stream_name,
                     int start_index,
                     int final_index,
                     export_stream_fcn_t fcn = NULL,
                     void *data = NULL);

  std::vector<pocolog_cpp::StreamDescription> getDescriptions();

  void loadStreamDescription(const std::string &stream_name,
                             pocolog_cpp::StreamDescription &desc);

private:
  std::vector<pocolog_cpp::StreamMetadata> getMetadata(pocolog_cpp::StreamDescription desc);

  pocolog_cpp::LogFile log_file_;
};

} // namespace rock_replay_cpp

#endif /* LogReader_hpp */
