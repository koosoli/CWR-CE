#pragma once

#include <memory>


namespace Poseidon
{
class IVoiceLoopbackBackend;
class VoNCapture;

class MicLoopback
{
  public:
    MicLoopback();
    ~MicLoopback();

    MicLoopback(const MicLoopback&) = delete;
    MicLoopback& operator=(const MicLoopback&) = delete;

    bool open(int sampleRate);
    void close();
    void tick(VoNCapture& capture);

    bool isOpen() const;

  private:
    IVoiceLoopbackBackend* EnsureImpl() const;

    mutable std::unique_ptr<IVoiceLoopbackBackend> _impl;
};

} // namespace Poseidon
