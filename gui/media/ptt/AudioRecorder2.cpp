#include "stdafx.h"
#include "AudioRecorder2.h"

#include "AudioUtils.h"
#include "AudioConverter.h"
#include "AmplitudeCalc.h"
#include "../../core_dispatcher.h"

namespace
{
    constexpr std::chrono::milliseconds timeout() noexcept { return std::chrono::milliseconds(100); };

    bool errorHappened(openal::ALCdevice *device)
    {
        if (openal::ALenum errCode = openal::alcGetError(device); errCode != ALC_NO_ERROR)
            return true;
        return false;
    }

    void stopDevice(openal::ALCdevice* device)
    {
        if (!device)
            return;
        openal::alcCaptureStop(device);
    }

    void startDevice(openal::ALCdevice* device)
    {
        if (!device)
            return;
        openal::alcCaptureStart(device);
    }

    void stopAndCloseDevice(openal::ALCdevice* device)
    {
        if (!device)
            return;
        openal::alcCaptureStop(device);
        openal::alcCaptureCloseDevice(device);
    }

    openal::ALCdevice* openDevice(size_t _bufferSize, std::string_view _name)
    {
        auto res = openal::alcCaptureOpenDevice(_name.empty() ? nullptr : _name.data(), ptt::AudioRecorder2::rate(), ptt::AudioRecorder2::format(), _bufferSize);
        if (res)
            openal::alcCaptureStart(res);
        return res;
    }

    bool inGuiThread() { return qApp->thread() == QThread::currentThread(); }

    QString getDeviceNameFromSettings()
    {
#ifndef STRIP_VOIP
        auto d = Ui::GetDispatcher()->getVoipController().activeDevice(voip_proxy::EvoipDevTypes::kvoipDevTypeAudioCapture);
        if (d)
            return QString::fromStdString((*d).name);
#endif
        return {};
    }

    bool areNamesEqual(const QString& n1, const QString& n2)
    {
        auto normalize = [](auto str)
        {
            str.remove(qsl("OpenAL Soft on"), Qt::CaseInsensitive);
            auto pred = [](auto ch)
            {
                if (ch.unicode() > 0xFFU)
                    return true;
                if (ch.isSpace())
                    return true;
                if (ch == ql1c('(') || ch == ql1c(')'))
                    return true;
                return false;
            };

            if (const auto it = std::find_if(std::as_const(str).begin(), std::as_const(str).end(), pred); it != std::as_const(str).end())
            {
                const auto idx = std::distance(std::as_const(str).begin(), it);
                const auto first = str.begin(); // implicit detach()
                const auto last = std::remove_if(first + idx, str.end(), pred);
                str.resize(last - first);
            }
            return str;
        };
        return normalize(n1) == normalize(n2);
    }

    std::optional<std::string> getDeviceName()
    {
        std::optional<std::string> result;
        const openal::ALCchar *device = openal::alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        const auto fromSettings = getDeviceNameFromSettings();
        if (fromSettings.isEmpty())
        {
            if (device && *device != '\0')
                result = std::string();
            return result;
        }

        size_t len = 0;
        while (device && *device != '\0')
        {
            len = strlen(device);
            if (areNamesEqual(fromSettings, QString::fromUtf8(device, len)))
            {
                result = std::string(device, len);
                return result;
            }
            qCDebug(pttLog) << device;
            device += (len + 1);
        }

        return result;
    }
}


namespace ptt
{
    AudioRecorder2::AudioRecorder2(QObject* _parent, const QString& _contact, std::chrono::seconds _maxDuration, std::chrono::seconds _minDuration)
        : QObject(_parent)
        , contact_(_contact)
        , maxDuration_(_maxDuration)
        , minDuration_(_minDuration)
        , timer_(new QTimer(this))
        , fft_(std::make_unique<AmplitudeCalc>(std::cref(buffer_), getWavHeaderSize(), rate()))
    {
        timer_->setInterval(timeout());
        internalBuffer_.resize(rate() * channelsCount() * bitesPerSample() / 8 * std::chrono::seconds(1).count(), std::byte(0));

        QObject::connect(timer_, &QTimer::timeout, this, &AudioRecorder2::onTimer);

        QObject::connect(this, &AudioRecorder2::recordS, this, &AudioRecorder2::recordImpl);
        QObject::connect(this, &AudioRecorder2::pauseRecordS, this, &AudioRecorder2::pauseRecordImpl);
        QObject::connect(this, &AudioRecorder2::stopS, this, &AudioRecorder2::stopImpl);
        QObject::connect(this, &AudioRecorder2::getDataS, this, &AudioRecorder2::getDataImpl);
        QObject::connect(this, &AudioRecorder2::getPcmDataS, this, &AudioRecorder2::getPcmDataImpl);
        QObject::connect(this, &AudioRecorder2::deviceListChangedS, this, [this]() { setReinit(true); });
        QObject::connect(this, &AudioRecorder2::dataReady, this, &AudioRecorder2::onDataReadyImpl);
        QObject::connect(this, &AudioRecorder2::getDurationS, this, [this]() { Q_EMIT durationChanged(currentDurationImpl(), contact_, QPrivateSignal()); });
        QObject::connect(this, &AudioRecorder2::clearS, this, &AudioRecorder2::clearImpl);
    }

    AudioRecorder2::~AudioRecorder2()
    {
        timer_->stop();
        if (device_)
        {
            stopAndCloseDevice(device_);
            device_ = nullptr;
        }
    }

    void AudioRecorder2::record()
    {
        qCDebug(pttLog) << "call record";
        Q_EMIT recordS(QPrivateSignal());
    }

    void AudioRecorder2::pauseRecord()
    {
        qCDebug(pttLog) << "call pause";
        Q_EMIT pauseRecordS(QPrivateSignal());
    }

     void AudioRecorder2::stop()
    {
        qCDebug(pttLog) << "call stop";
        Q_EMIT stopS(QPrivateSignal());
    }

    void AudioRecorder2::getData(ptt::StatInfo&& _statInfo)
    {
        qCDebug(pttLog) << "call getData";
        Q_EMIT getDataS(_statInfo, QPrivateSignal());
    }

    void AudioRecorder2::deviceListChanged()
    {
        Q_EMIT deviceListChangedS(QPrivateSignal());
    }

    void AudioRecorder2::getDuration()
    {
        Q_EMIT getDurationS(QPrivateSignal());
    }

    void AudioRecorder2::getPcmData()
    {
        Q_EMIT getPcmDataS(QPrivateSignal());
    }

    void AudioRecorder2::clear()
    {
        qCDebug(pttLog) << "call clear";
        Q_EMIT clearS(QPrivateSignal());
    }

    void AudioRecorder2::onTimer()
    {
        im_assert(!inGuiThread());

        if (needReinit())
        {
            setReinit(false);
            if (!reInit())
                return;
        }

        if (!device_)
            return;

        if (const auto s = getStateImpl(); s == State2::Stopped || s == State2::Paused)
            return;

        openal::ALint sample = 0;
        openal::alcGetIntegerv(device_, ALC_CAPTURE_SAMPLES, (openal::ALCsizei)sizeof(openal::ALint), &sample);
        if (errorHappened(device_))
        {
            stopImpl();
            Q_EMIT error(Error2::DeviceInit, contact_, QPrivateSignal());
            return;
        }
        if (sample <= 0)
            return;

        try
        {
            if (const auto size = internalBuffer_.size(); size < std::size_t(sample) * sizeof(openal::ALCushort) * channelsCount())
                internalBuffer_.resize(std::size_t(sample) * sizeof(openal::ALCushort) * channelsCount(), std::byte(0));
            openal::alcCaptureSamples(device_, (openal::ALCvoid *)internalBuffer_.data(), sample);

            buffer_.append(reinterpret_cast<const char*>(internalBuffer_.data()), sample * sizeof(openal::ALCushort) * channelsCount());
            if (auto v = fft_->getSamples(); !v.isEmpty())
                Q_EMIT spectrum(v, contact_, QPrivateSignal());
        }
        catch (const std::bad_alloc&)
        {
            stopImpl();
            Q_EMIT error(Error2::BufferOOM, contact_, QPrivateSignal());
            return;
        }

        const auto newDuration = calculateDuration(buffer_.size() - getWavHeaderSize(), rate(), channelsCount(), bitesPerSample());
        setDurationImpl(newDuration);

        if (newDuration >= maxDuration_)
        {
            if constexpr (isLoopRecordingAvailable())
            {
                insertWavHeader();

                Q_EMIT dataReady(buffer_, contact_, ptt::StatInfo{}, QPrivateSignal());
                resetBuffer();
            }
            else
            {
                stopImpl();
                Q_EMIT limitReached(contact_, QPrivateSignal());
            }
        }
    }

    void AudioRecorder2::resetBuffer()
    {
        im_assert(!inGuiThread());
        buffer_.clear();
        for (size_t i = 0, size = getWavHeaderSize(); i < size; ++i)
            buffer_.push_back('\0');

        setDurationImpl(std::chrono::seconds::zero());

        fft_->reset();
    }

    bool AudioRecorder2::insertWavHeader()
    {
        const auto pcmSize = size_t(buffer_.size()) - getWavHeaderSize();
        if (pcmSize <= 0)
            return false;

        const auto wavHeaderData = getWavHeaderData(pcmSize, rate(), channelsCount(), bitesPerSample());
#ifdef _WIN32
        memcpy_s(reinterpret_cast<void*>(buffer_.data()), wavHeaderData.size(), reinterpret_cast<const void*>(wavHeaderData.data()), wavHeaderData.size());
#else
        memcpy(reinterpret_cast<void*>(buffer_.data()), reinterpret_cast<const void*>(wavHeaderData.data()), wavHeaderData.size());
#endif
        return true;
    }

    bool AudioRecorder2::needReinit() const noexcept
    {
        return needReinitDevice_;
    }

    void AudioRecorder2::setReinit(bool _v)
    {
        needReinitDevice_ = _v;
    }

    bool AudioRecorder2::reInit()
    {
        qCDebug(pttLog) << "reInit";
        if (device_)
            stopAndCloseDevice(device_);
        deviceName_ = getDeviceName().value_or(std::string());
        device_ = openDevice(internalBuffer_.size(), deviceName_);
        if (!device_)
        {
            setStateImpl(State2::Stopped);
            timer_->stop();
            Q_EMIT error(Error2::DeviceInit, contact_, QPrivateSignal());
            return false;
        }
        return true;
    }

    void AudioRecorder2::setStateImpl(State2 _state)
    {
        im_assert(!inGuiThread());
        if (state_ != _state)
        {
            qCDebug(pttLog) << "setStateImpl" << (int)_state;
            state_ = _state;
            Q_EMIT stateChanged(_state, contact_, QPrivateSignal());
        }
    }

    State2 AudioRecorder2::getStateImpl() const
    {
        return state_;
    }

    std::chrono::milliseconds AudioRecorder2::currentDurationImpl() const
    {
        return currentDurationImpl_;
    }

    void AudioRecorder2::setDurationImpl(std::chrono::milliseconds _duration)
    {
        if (currentDurationImpl_ != _duration)
        {
            currentDurationImpl_ = _duration;
            Q_EMIT durationChanged(currentDurationImpl_, contact_, QPrivateSignal());
        }
    }

    bool AudioRecorder2::hasDevice()
    {
        const openal::ALCchar *device = openal::alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        return device && *device != '\0'; // have at least 1 device
    }

    void AudioRecorder2::recordImpl()
    {
        im_assert(!inGuiThread());
        if (timer_->isActive())
            return;

        if constexpr (platform::is_apple())
        {
            if (getStateImpl() == State2::Stopped)
                setReinit(true);
        }

        if (needReinit() || (deviceName_ != getDeviceName()))
        {
            setReinit(false);
            if (!reInit())
                return;
        }
        if (getStateImpl() == State2::Stopped)
            resetBuffer();
        stopDevice(device_);
        startDevice(device_);
        setStateImpl(State2::Recording);
        onTimer();
        timer_->start();
    }

    void AudioRecorder2::pauseRecordImpl()
    {
        im_assert(!inGuiThread());

        setStateImpl(State2::Paused);
        timer_->stop();
        stopDevice(device_);
    }

    void AudioRecorder2::stopImpl()
    {
        im_assert(!inGuiThread());

        const auto prevState = getStateImpl();
        setStateImpl(State2::Stopped);
        timer_->stop();
        stopDevice(device_);

        if ((prevState == State2::Recording || prevState == State2::Paused) && (currentDurationImpl() < minDuration_))
            Q_EMIT tooShortRecord(contact_, QPrivateSignal());
    }

    void AudioRecorder2::getDataImpl(const ptt::StatInfo& _statInfo)
    {
        im_assert(!inGuiThread());

        if ((currentDurationImpl() >= minDuration_) && insertWavHeader())
            Q_EMIT dataReady(buffer_, contact_, _statInfo, QPrivateSignal());
    }

    void AudioRecorder2::getPcmDataImpl()
    {
        im_assert(!inGuiThread());

        if ((currentDurationImpl() >= minDuration_) && insertWavHeader())
            Q_EMIT pcmDataReady({ buffer_, getWavHeaderSize(), rate(), format(), bitesPerSample(), channelsCount() }, contact_, QPrivateSignal());
    }

    void AudioRecorder2::onDataReadyImpl(const QByteArray& _data, const QString&, const ptt::StatInfo& _statInfo)
    {
        qCDebug(pttLog) << "onDataRecieved" << _data.size();

        auto task = new ConvertTask(_data, rate(), channelsCount(), bitesPerSample());

#ifdef DUMP_PTT_WAV
        if (QFile f(qApp->applicationDirPath() % QDateTime::currentDateTimeUtc().toString() % u".wav"); f.open(QIODevice::WriteOnly))
            f.write(_data);
#endif

        QObject::connect(task, &ConvertTask::ready, this, [this, _statInfo](const QString& _aacFile, std::chrono::seconds duration)
        {
            auto stat = _statInfo;
            stat.duration = duration;
            Q_EMIT aacReady(contact_, _aacFile, duration, stat, QPrivateSignal());
        });

        QObject::connect(task, &ConvertTask::error, this, [this]()
        {
            qCDebug(pttLog) << "error";
            Q_EMIT error(Error2::Convert, contact_, QPrivateSignal());
        });

        QThreadPool::globalInstance()->start(task);
    }

    void AudioRecorder2::clearImpl()
    {
        resetBuffer();
    }
}
