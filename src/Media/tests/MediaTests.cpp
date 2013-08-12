#include "Sourcey/Application.h"
#include "Sourcey/Logger.h"
#include "Sourcey/Signal.h"
#include "Sourcey/Queue.h"
#include "Sourcey/PacketQueue.h"
#include "Sourcey/Media/FLVMetadataInjector.h"
#include "Sourcey/Media/FormatRegistry.h"
#include "Sourcey/Media/MediaFactory.h"
#include "Sourcey/Media/AudioCapture.h"
#include "Sourcey/Media/AVPacketEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/fifo.h>
#include <libswscale/swscale.h>
}
/*
#include "Sourcey/Timer.h"
#include "Sourcey/Net/UDPSocket.h"
#include "Sourcey/Net/TCPSocket.h" //Client


#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#include <assert.h>
#include <stdio.h>


// Detect Memory Leaks
#ifdef _DEBUG
#include "MemLeakDetect/MemLeakDetect.h"
#include "MemLeakDetect/MemLeakDetect.cpp"
CMemLeakDetect memLeakDetect;
#endif
*/


using namespace std;
using namespace scy;
using namespace scy::av;
using namespace cv;


namespace scy {
namespace av {


class Tests
{		
public:

	Tests() 
	{   
		debugL() << "Running tests..." << endl;	

		//videoCapture = nil;
		//audioCapture = nil;
		//
		// Set up our devices
		MediaFactory::initialize();
		MediaFactory::instance()->loadVideo();

		try
		{			
			//MediaFactory::instance()->devices().print(cout);
			//audioCapture = new AudioCapture(device.id, 2, 44100);
			
			//videoCapture = new VideoCapture(0, true);
			//audioCapture = new AudioCapture(0, 1, 16000);	
			//audioCapture = new AudioCapture(0, 1, 11025);	
			//for (int i = 0; i < 100; i++) { //0000000000
			//	runAudioCaptureTest();
			//}
			//runAudioCaptureThreadTest();
			//runVideoCaptureThreadTest();
			//runAudioCaptureTest();
			//runOpenCVMJPEGTest();
			//runVideoRecorderTest();
			//runAudioRecorderTest();
			//runCaptureEncoderTest();
			//runAVEncoderTest();
			runStreamEncoderTest();
			//runMediaSocketTest();
			//runFormatFactoryTest();
			//runMotionDetectorTest();
			//runBuildXMLString();
		
			//runStreamEncoderTest();
			//runOpenCVCaptureTest();
			//runDirectShowCaptureTest();
		}
		catch (Exception& exc)
		{
			errorL() << "Error: " << exc.message() << endl;
		}		

		MediaFactory::instance()->unloadVideo();
		MediaFactory::uninitialize();

		//if (videoCapture)
		//	delete audioCapture;
		//if (videoCapture)
		//	delete videoCapture;		

		debugL() << "Tests Exiting..." << endl;	
	};
	
		
	// ---------------------------------------------------------------------
	// Packet Stream Encoder Test
	//
	class StreamEncoderTest
	{
	public:
		StreamEncoderTest(const av::RecordingOptions& opts) : 
			closed(false), options(opts), frames(0), videoCapture(nil), audioCapture(nil)
		{
			ofile.open("enctest1.mp4", ios::out | ios::binary);
			assert(ofile.is_open());

			// Init captures
			if (options.oformat.video.enabled) {
				videoCapture = MediaFactory::instance()->getVideoCapture(0);
				setVideoCaptureInputFormat(videoCapture, options.iformat);	
				stream.attachSource(videoCapture, false, true);
			}
			if (options.oformat.audio.enabled) {
				Device device;
				if (MediaFactory::instance()->devices().getDefaultAudioInputDevice(device)) {
					debugL("StreamEncoderTest") << "Audio Device: " << device.id << endl;
					audioCapture = MediaFactory::instance()->createAudioCapture(device.id,
						options.oformat.audio.channels, 
						options.oformat.audio.sampleRate);
					stream.attachSource(audioCapture, true, true);
				}
				else assert(0);
			}			
								
			// Init as async queue for testing
			queue = new AsyncPacketQueue;
			stream.attach(queue, 3, true);

			// Init encoder				
			encoder = new AVPacketEncoder(options);
			stream.attach(encoder, 5, true);

			// Start the stream
			stream.start();
			stream.emitter() += packetDelegate(this, &StreamEncoderTest::onVideoEncoded);	
		}		
		
		virtual ~StreamEncoderTest()
		{		
			debugL("StreamEncoderTest") << "Destroying" << endl;
			close();
			debugL("StreamEncoderTest") << "Destroying: OK" << endl;
		}

		void close()
		{
			debugL("StreamEncoderTest") << "########### Closing: " << frames << endl;
			closed = true;
			
			// Close the stream
			// This will flush any queued items
			stream.close();
			
			// Make sure everything shutdown properly
			assert(queue->queue().empty());
			assert(encoder->isStopped());
			
			// Close the output file
			ofile.close();
		}


		void onVideoEncoded(void* sender, RawPacket& packet)
		{
			debugL("StreamEncoderTest") << "########### On packet: " << packet.size() << endl;
			frames++;
			assert(!closed);
			assert(packet.array());
			assert(packet.size());
			ofile.write(packet.array(), packet.size());
			//assert(frames <= 3);
			//if (frames == 20)
			//	close();
		}
			
		/*
		// Manually stop captures
		videoCapture->Emitter.clear();
		videoCapture->stop();
		if (audioCapture) {
			audioCapture->Emitter.clear();
			audioCapture->stop();
		}

		//stream.waitForReady();
		*/
		
		int frames;
		bool closed;
		std::ofstream ofile;
		VideoCapture* videoCapture;
		AudioCapture* audioCapture;
		AsyncPacketQueue* queue;
		AVPacketEncoder* encoder;
		av::RecordingOptions options;
		PacketStream stream;
	};	
				
	static void onShutdownSignal(void* opaque)
	{
		std::vector<StreamEncoderTest*>& tests = *reinterpret_cast<std::vector<StreamEncoderTest*>*>(opaque);

		for (unsigned i = 0; i < tests.size(); i++) {
			// Delete the pointer directly to 
			// ensure synchronization is golden.
			delete tests[i];
		}
	}

	void runStreamEncoderTest()
	{
		debugL("StreamEncoderTest") << "Running" << endl;	
		try
		{
			// Setup encoding format
			Format mp4(Format("MP4", "mp4", 
				//VideoCodec("MPEG4", "mpeg4", 640, 480, 20), 
				VideoCodec("H264", "libx264", 320, 240, 10)//,
				//AudioCodec("AAC", "aac", 2, 44100)
				//AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)
				//AudioCodec("MP3", "libmp3lame", 2, 44100, 64000)
				//AudioCodec("AC3", "ac3_fixed", 2, 44100, 64000)
			));

			Format mp3("MP3", "mp3", 
				AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)); 
		
			av::RecordingOptions options;		
			//options.ofile = "enctest.mp3";
			options.ofile = "enctest2.mp4";	
			//options.ofile = "enctest.mjpeg";	
			options.oformat = mp4;

			// Initialize test runners
			int numTests = 1;
			std::vector<StreamEncoderTest*> threads;
			for (unsigned i = 0; i < numTests; i++) 
				threads.push_back(new StreamEncoderTest(options));

			// Run until Ctrl-C is pressed
			Application app;
			app.waitForShutdown(onShutdownSignal, &threads);
			
			// Shutdown the garbage collector so we can free memory.
			//GarbageCollector::instance().shutdown();
			debugL("Tests") << "#################### Finalizing" << endl;
			GarbageCollector::instance().shutdown();
			debugL("Tests") << "#################### Exiting" << endl;

			//debugL("Tests") << "#################### Finalizing" << endl;
			//app.cleanup();
			//debugL("Tests") << "#################### Exiting" << endl;

			// Wait for enter keypress
			//scy::pause();
		}
		catch (Exception& exc)
		{
			errorL("StreamEncoderTest") << "Error: " << exc.message() << endl;
		}

		debugL("StreamEncoderTest") << "Ended" << endl;
	}
	

			// Attach a few adapters to better test destruction synchronization
			//stream.attach(new FPSLimiter(100), 6, true);
			//stream.attach(new FPSLimiter(100), 7, true);
			//stream.attach(new FPSLimiter(3), 4, true);
			// For best practice we would detach the delegate,
			// but we want to make sure there are no late packets 
			// arriving after the stream is closed.
			//stream.detach(packetDelegate(this, &StreamEncoderTest::onVideoEncoded));		
			/*
			// Manually flush queue
			while (!queue->_queue.empty()) {				
				int s = queue->_queue.size();
				//queue->run(); // flush pending
				debugL("StreamEncoderTest") << "########### Waiting for queue: " << s << endl;
				scy::sleep(50);
			}
			assert(queue->_queue.empty());
			queue->cancel();
			//queue->clear();

			// Close the encoder
			encoder->uninitialize();	
			*/	

			
	// ---------------------------------------------------------------------
	// Capture Encoder Test
	//		
	class CaptureEncoder {
	public:
		CaptureEncoder(ICapture* capture, const av::RecordingOptions& options) : 
			capture(capture), encoder(options), closed(false) {
			assert(capture);	
			encoder.initialize();
		};

		void start()
		{
			capture->emitter() += packetDelegate(this, &CaptureEncoder::onFrame);	
			capture->start();
		}

		void stop()
		{
			capture->emitter() -= packetDelegate(this, &CaptureEncoder::onFrame);	
			capture->stop();
			encoder.uninitialize();
			closed = true;
		}

		void onFrame(void* sender, RawPacket& packet)
		{
			debugL("CaptureEncoder") << "On packet: " << packet.size() << endl;
			assert(!closed);
			try 
			{	
				assert(0);
				//encoder.process(packet);
			}
			catch (Exception& exc) 
			{
				errorL("CaptureEncoder") << "#######################: " << exc.message() << endl;
				stop();
			}
		}

		bool closed;
		ICapture* capture;
		AVEncoder encoder;
	};


	void runCaptureEncoderTest()
	{
		debugL("CaptureEncoderTest") << "Starting" << endl;	
		
		av::VideoCapture capture(0);

		// Setup encoding format
		Format mp4(Format("MP4", "mp4", 
			VideoCodec("MPEG4", "mpeg4", 640, 480, 10)//, 
			//VideoCodec("H264", "libx264", 320, 240, 25),
			//AudioCodec("AAC", "aac", 2, 44100)
			//AudioCodec("MP3", "libmp3lame", 2, 44100, 64000)
			//AudioCodec("AC3", "ac3_fixed", 2, 44100, 64000)
		));
		
		av::RecordingOptions options;			
		options.ofile = "enctest.mp4"; // enctest.mjpeg
		options.oformat = mp4;
		setVideoCaptureInputFormat(&capture, options.iformat);	
		
		CaptureEncoder encoder(&capture, options);
		encoder.start();

		std::puts("Press any key to continue...");
		std::getchar();

		encoder.stop();

		debugL("CaptureEncoderTest") << "Complete" << endl;	
	}


	// ---------------------------------------------------------------------
	// OpenCV Capture Test
	//	
	void runOpenCVCaptureTest()
	{
		debugL("StreamEncoderTest") << "Starting" << endl;	

		cv::VideoCapture cap(0);
		if (!cap.isOpened())
			assert(false);

		cv::Mat edges;
		cv::namedWindow("edges",1);
		for(;;) {
			cv::Mat frame;
			cap >> frame; // get a new frame from camera
			cv::cvtColor(frame, edges, CV_BGR2GRAY);
			cv::GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
			cv::Canny(edges, edges, 0, 30, 3);
			cv::imshow("edges", edges);
			if (cv::waitKey(30) >= 0) break;
		}

		debugL("StreamEncoderTest") << "Complete" << endl;	
	}
	
	
		/*
	// ---------------------------------------------------------------------
	// Video Capture Thread Test
	//	
	class VideoCaptureThread: public abstract::Runnable
	{
	public:
		VideoCaptureThread(int deviceID, const string& name = "Capture Thread") : 
			closed(false),
			_deviceID(deviceID),
			frames(0)
		{	
			_thread.setName(name);
			_thread.start(*this);
		}		

		VideoCaptureThread(const string& filename, const string& name = "Capture Thread") : 
			closed(false),
			_filename(filename),
			_deviceID(0),
			frames(0)
		{	
			_thread.setName(name);
			_thread.start(*this);
		}	
		
		~VideoCaptureThread()
		{
			closed = true;
			_thread.join();
		}

		void run()
		{
			try
			{				
				VideoCapture* capture = !_filename.empty() ?
					MediaFactory::instance()->createFileCapture(_filename) : //, true
					MediaFactory::instance()->getVideoCapture(_deviceID);
				capture->attach(packetDelegate(this, &VideoCaptureThread::onVideo));	
				capture->start();

				while (!closed)
				{
					cv::waitKey(50);
				}
				
				capture->detach(packetDelegate(this, &VideoCaptureThread::onVideo));	

				debugL("VideoCaptureThread") << " Ending.................." << endl;
			}
			catch (Exception& exc)
			{
				Log("error") << "[VideoCaptureThread] Error: " << exc.message() << endl;
			}
			
			debugL("VideoCaptureThread") << " Ended.................." << endl;
			//delete this;
		}

		void onVideo(void* sender, MatPacket& packet)
		{
			debugL() << "VideoCaptureThread: On Packet: " << packet.size() << endl;
			//debugL() << "VideoCaptureThread: On Packet 1: " << packet.mat->total() << endl;
			//debugL() << "VideoCaptureThread: On Packet 1: " << packet.mat->cols << endl;
			//assert(packet.mat->cols);
			//assert(packet.mat->rows);
			cv::imshow(_thread.name(), *packet.mat);
			frames++;
		}
		
		Thread _thread;
		std::string _filename;
		int	_deviceID;
		int frames;
		bool closed;
	};


	void runVideoCaptureThreadTest()
	{
		debugL() << "Running video capture test..." << endl;	
				
		// start and destroy multiple receivers
		list<VideoCaptureThread*> threads;
		for (int i = 0; i < 3; i++) { //0000000000			
			threads.push_back(new VideoCaptureThread(0)); //, Poco::format("Capture Thread %d", i)
			//threads.push_back(new VideoCaptureThread("D:\\dev\\lib\\ffmpeg-1.0.1-gpl\\bin\\output.avi", Poco::format("Capture Thread %d", i)));
		}

		//util::pause();

		util::clearList(threads);
	}
	
	
	// ---------------------------------------------------------------------
	// Audio Capture Thread Test
	//
	class AudioCaptureThread: public abstract::Runnable
	{
	public:
		AudioCaptureThread(const string& name = "Capture Thread")
		{	
			_thread.setName(name);
			_thread.start(*this);
		}		
		
		~AudioCaptureThread()
		{
			_wakeUp.set();
			_thread.join();
		}

		void run()
		{
			try
			{
				//capture = new AudioCapture(0, 2, 44100);
				//capture = new AudioCapture(0, 1, 16000);	
				//capture = new AudioCapture(0, 1, 11025);	
				AudioCapture* capture = new AudioCapture(0, 1, 16000);
				capture->attach(audioDelegate(this, &AudioCaptureThread::onAudio));	
				
				_wakeUp.wait();
				
				capture->detach(audioDelegate(this, &AudioCaptureThread::onAudio));	
				delete capture;
				
				debugL() << "[AudioCaptureThread] Ending.................." << endl;
			}
			catch (Exception& exc)
			{
				Log("error") << "[AudioCaptureThread] Error: " << exc.message() << endl;
			}
			
			debugL() << "[AudioCaptureThread] Ended.................." << endl;
			//delete this;
		}

		void onAudio(void* sender, AudioPacket& packet)
		{
			debugL() << "[AudioCaptureThread] On Packet: " << packet.size() << endl;
			//cv::imshow(_thread.name(), *packet.mat);
		}
		
		Thread	_thread;
		Poco::Event		_wakeUp;
		int				frames;
	};


	void runAudioCaptureThreadTest()
	{
		debugL() << "Running Audio Capture Thread test..." << endl;	
				
		// start and destroy multiple receivers
		list<AudioCaptureThread*> threads;
		for (int i = 0; i < 10; i++) { //0000000000
			threads.push_back(new AudioCaptureThread()); //Poco::format("Audio Capture Thread %d", i))
		}

		//util::pause();

		util::clearList(threads);
	}

	
	// ---------------------------------------------------------------------
	// Audio Capture Test
	//
	void onCaptureTestAudioCapture(void*, AudioPacket& packet)
	{
		debugL() << "onCaptureTestAudioCapture: " << packet.size() << endl;
		//cv::imshow("Target", *packet.mat);
	}	

	void runAudioCaptureTest()
	{
		debugL() << "Running Audio Capture test..." << endl;	
		
		AudioCapture* capture = new AudioCapture(0, 1, 16000);
		capture->attach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		//Util::pause();
		capture->detach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		delete capture;

		AudioCapture* capture = new AudioCapture(0, 1, 16000);
		capture->attach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));	
		//audioCapture->start();
		
		//Util::pause();

		capture->detach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		delete capture;
		//audioCapture->stop();
	}
		*/
	
		/*
		//tests[i]->close();
		//debugL("StreamEncoderTest") << "########### Closing: " 
		//	<< i << ": " << tests[i]->frames << ": " << tests[i]->closed << endl;
		//assert(tests[i]->closed);
		// start and destroy multiple receivers
		list<StreamEncoderTest*> threads;
		for (unsigned i = 0; i < 1; i++) { //0000000000
			//Log("debug") << "StreamEncoderTest: " << _name << endl;
			threads.push_back(new StreamEncoderTest);
			//StreamEncoderTest test;
			//UDPSocket sock;
		}
		//util::pause();
		debugL() << "Stream Encoder Test: Cleanup" << endl;	
		util::clearList(threads);
		thread.run();
		*/
	
				/*
				options.oformat = Format("MJPEG High", "mjpeg", VideoCodec("mjpeg", "mjpeg", 640, 480, 10));		
				options.oformat.video.pixelFmt = "yuvj420p";
				options.oformat.video.quality = 100;

				//options.oformat = Format("MP4", "mpeg4",
					//VideoCodec("MPEG4", "mpeg4", 640, 480, 25), 
					//VideoCodec("H264", "libx264", 320, 240, 25),
					//AudioCodec("AC3", "ac3_fixed", 2, 44100));
			
				//options.oformat  = Format("MP3", "mp3", 
				//	VideoCodec(), 
				//	AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)); //)

				//options.oformat = Format("MP4", Format::MP4, VideoCodec(Codec::MPEG4, "MPEG4", 1024, 768, 25));
				//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 1024, 768, 25));
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
				//options.oformat = Format("FLV", "flv", 
				//	VideoCodec(Codec::FLV, "FLV", 400, 300, 15)//,
				//	//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025)
				//	);
				//options.ofile = "av_capture_test.mp4";
				*/
	
	/*
	// ---------------------------------------------------------------------
	//
	// Video Media Socket Test
	//
	// ---------------------------------------------------------------------	

	class MediaConnection: public TCPServerConnection
	{
	public:
		MediaConnection(const StreamSocket& s) : 
		  TCPServerConnection(s), stop(false)//, lastTimestamp(0), timestampGap(0), waitForKeyFrame(true)
		{		
		}
		
		void run()
		{
			try
			{
				av::RecordingOptions options;
				//options.ofile = "enctest.mp4";
				//options.stopAt = time(0) + 3;
				av::setVideoCaptureInputForma(videoCapture, options.iformat);	
				//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 1024, 768, 25));
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
				options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 640, 480, 100));	
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 320, 240, 15));	
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));		
					

				//options.iformat.video.pixfmt = (scy::av::PixelFormat::ID)PIX_FMT_GRAY8; //PIX_FMT_BGR8; //PIX_FMT_BGR32 // PIX_FMT_BGR32
				//MotionDetector* detector = new MotionDetector();
				//detector->setVideoCapture(videoCapture);
				//stream.attach(detector, true);		
				//stream.attach(new SurveillanceMJPEGPacketizer(*detector), 20, true);	

				stream.attach(videoCapture, false);

				stream.attach(packetDelegate(this, &MediaConnection::onVideoEncoded));
		
				// init encoder				
				AVEncoder* encoder = new AVEncoder();
				encoder->setParams(options);
				encoder->initialize();
				stream.attach(encoder, 5, true);				
				
				//HTTPMultipartAdapter* packetizer = new HTTPMultipartAdapter("image/jpeg");
				//stream.attach(packetizer);

				//FLVMetadataInjector* injector = new FLVMetadataInjector(options.oformat);
				//stream.attach(injector);

				// start the stream
				stream.start();

				while (!stop)
				{
					Thread::sleep(50);
				}
				
				//stream.detach(packetDelegate(this, &MediaConnection::onVideoEncoded));
				//stream.stop();

				//outputFile.close();
				cerr << "MediaConnection: ENDING!!!!!!!!!!!!!" << endl;
			}
			catch (Exception& exc)
			{
				cerr << "MediaConnection: " << exc.message() << endl;
			}
		}

		void onVideoEncoded(void* sender, RawPacket& packet)
		{
			StreamSocket& ss = socket();

			fpsCounter.tick();
			debugL() << "On Video Packet Encoded: " << fpsCounter.fps << endl;

			//if (fpsCounter.frames < 10)
			//	return;
			//if (fpsCounter.frames == 10) {
			//	stream.reset();
			//	return;
			//}

			try
			{		
				ss.sendBytes(packet.data, packet.size);
			}
			catch (Exception& exc)
			{
				cerr << "MediaConnection: " << exc.message() << endl;
				stop = true;
			}
		}
		
		bool stop;
		PacketStream stream;
		FPSCounter fpsCounter;
	};

	void runMediaSocketTest()
	{		
		debugL() << "Running Media Socket Test" << endl;
		
		ServerSocket svs(666);
		TCPServer srv(new TCPServerConnectionFactoryImpl<MediaConnection>(), svs);
		srv.start();
		//util::pause();
	}

	
	// ---------------------------------------------------------------------
	//
	// Video CaptureEncoder Test
	//
	// ---------------------------------------------------------------------
	//UDPSocket outputSocket;

	void runCaptureEncoderTest()
	{		
		debugL() << "Running Capture Encoder Test" << endl;	

		av::RecordingOptions options;
		options.ofile = "enctest.mp4";
		//options.stopAt = time(0) + 3;
		av::setVideoCaptureInputForma(videoCapture, options.iformat);	
		//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 400, 300));
		options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
		//options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 320, 240, 15));	
				
		//CaptureEncoder<VideoEncoder> enc(videoCapture, options);	
		//encoder = new AVEncoder(stream.options());
		CaptureRecorder enc(options, videoCapture, audioCapture);		
		//enc.initialize();	
		
		enc.attach(packetDelegate(this, &Tests::onCaptureEncoderTestVideoEncoded));
		enc.start();
		//util::pause();
		enc.stop();

		debugL() << "Running Capture Encoder Test: END" << endl;	
	}
	
	FPSCounter fpsCounter;
	void onCaptureEncoderTestVideoEncoded(void* sender, MediaPacket& packet)
	{
		fpsCounter.tick();
		debugL() << "On Video Packet Encoded: " << fpsCounter.fps << endl;
	}

	// ---------------------------------------------------------------------
	//
	// Video CaptureRecorder Test
	//
	// ---------------------------------------------------------------------
	void runVideoRecorderTest()
	{
		av::RecordingOptions options;
		options.ofile = "av_capture_test.flv";
		
		options.oformat = Format("FLV", "flv", 
			VideoCodec(Codec::FLV, "FLV", 320, 240, 15),
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025),
			AudioCodec(Codec::Speex, "Speex", 1, 16000)//,
			//AudioCodec(Codec::Speex, "Speex", 2, 44100)
			);	
		//options.oformat = Format("MP4", Format::MP4,
		options.oformat = Format("FLV", "flv",
			//VideoCodec(Codec::MPEG4, "MPEG4", 640, 480, 25), 
			//VideoCodec(Codec::H264, "H264", 640, 480, 25), 
			VideoCodec(Codec::FLV, "FLV", 640, 480, 25), 
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025)
			//AudioCodec(Codec::Speex, "Speex", 2, 44100)
			//AudioCodec(Codec::MP3, "MP3", 2, 44100)
			//AudioCodec(Codec::AAC, "AAC", 2, 44100)
			AudioCodec(Codec::AAC, "AAC", 1, 11025)
		);

		options.oformat = Format("M4A", Format::M4A,
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 44100)
			AudioCodec(Codec::AAC, "AAC", 2, 44100)
			//AudioCodec(Codec::AC3, "AC3", 2, 44100)
		);



		//options.stopAt = time(0) + 5; // Max 24 hours		
		av::setVideoCaptureInputForma(videoCapture, options.iformat);	

		CaptureRecorder enc(options, nil, audioCapture); //videoCapture
			
		audioCapture->start();	
		enc.start();
		//util::pause();
		enc.stop();
	}

	// ---------------------------------------------------------------------
	//
	// Audio CaptureRecorder Test
	//
	// ---------------------------------------------------------------------
	void runAudioRecorderTest()
	{
		av::RecordingOptions options;
		options.ofile = "audio_test.mp3";
		options.stopAt = time(0) + 5;
		options.oformat = Format("MP3", "mp3",
			AudioCodec(Codec::MP3, "MP3", 2, 44100)
		);

		CaptureRecorder enc(options, nil, audioCapture);
		
		audioCapture->start();	
		enc.start();
		//util::pause();
		enc.stop();
	}
			
	*/

};


} // namespace av
} // namespace scy


int main(int argc, char** argv)
{
	Logger::instance().add(new ConsoleChannel("debug", TraceLevel));
	
	// Set up our devices
	//MediaFactory::initialize();
	//MediaFactory::instance()->loadVideo();
	MediaFactory::instance()->loadVideo();	
	{
		Tests run;	
		//util::pause();
	}		
	MediaFactory::instance()->unloadVideo();
	//MediaFactory::instance()->unloadAudio();
	//MediaFactory::uninitialize();
		
	Logger::shutdown();
	return 0;
}



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

// 5 seconds stream duration */
#define STREAM_DURATION   3.0
#define STREAM_FRAME_RATE 25 // 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT    PIX_FMT_YUV420P // default pix_fmt */

static int sws_flags = SWS_BICUBIC;

/**************************************************************/
// audio output */

static float t, tincr, tincr2;
static int16_t *samples;
//static int audio_input_frame_size;

static AVFifoBuffer*	audio_fifo;		
static uint8_t*			audio_buffer;

static int				audio_input_frame_size;
static int				audio_output_frame_size;

cv::VideoCapture __cap(0);
cv::Mat __frame;


typedef signed short  MY_TYPE;
#define FORMAT RTAUDIO_SINT16


struct APacket {
	MY_TYPE* data;
	int size;
	double time;
};

scy::Queue<APacket> _audioQueue;
scy::Queue<VideoPacket> _videoQueue;


static void open_audio(AVFormatContext *oc, AVStream *st,  AVCodec *codec)
{
    AVCodecContext *c;

    c = st->codec;

    // open it */
    if (avcodec_open2(c, codec, nil) < 0) {
        fprintf(stderr, "could not open codec\n");
        assert(0);
    }

    // init signal generator */
    t     = 0;
    tincr = 2 * M_PI * 110.0 / c->sample_rate;
    // increment frequency by 110 Hz per second */
    tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
        audio_input_frame_size = 10000;
    else
        audio_input_frame_size = c->frame_size;

	audio_output_frame_size = av_samples_get_buffer_size(
		nil, c->channels, audio_input_frame_size, c->sample_fmt, 1);

	assert(audio_input_frame_size > 0);
	assert(audio_output_frame_size > 0);

	// The encoder may require a minimum number of raw audio
	// samples for each encoding but we can't guarantee we'll
	// get this minimum each time an audio frame is decoded
	// from the in file, so we use a FIFO to store up incoming
	// raw samples until we have enough to call the codec.
	audio_fifo = av_fifo_alloc(audio_output_frame_size * 2);

	// Allocate a buffer to read OUT of the FIFO into. 
	// The FIFO maintains its own buffer internally.
	audio_buffer = (uint8_t*)av_malloc(audio_output_frame_size);


    samples = (int16_t *)av_malloc(audio_input_frame_size *
                        av_get_bytes_per_sample(c->sample_fmt) *
                        c->channels);
}

/*
 * add an audio output stream
 */
static AVStream *add_audio_stream(AVFormatContext *oc, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;
    AVCodec *codec;

    // find the audio encoder */
	//codec = avcodec_find_encoder_by_name("ac3_fixed"); //libx264
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        assert(0);
    }

    st = avformat_new_stream(oc, codec);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        assert(0);
    }

    c = st->codec;

    // put sample parameters */
    c->sample_fmt  = AV_SAMPLE_FMT_S16;
    c->bit_rate    = 64000;
    c->sample_rate = 44100;
    c->channels    = 2;

    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;


	
    open_audio(oc, st, codec);
    return st;
}

// Prepare a 16 bit dummy audio frame of 'frame_size' samples and
// 'nb_channels' channels. 
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        t     += tincr;
        tincr += tincr2;
    }
}

static void write_audio_frame(AVFormatContext *oc, AVStream *st, APacket& packet) //uint8_t* buffer, int bufferSize
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame = avcodec_alloc_frame();
    int got_packet;
	
	traceL("AVEncoder") << "Audio Input: " << packet.size << endl;
	//traceL("AVEncoder") << "Audio Input Size: " << audio_input_frame_size << endl;(uint8_t*)
	
	av_fifo_generic_write(audio_fifo, packet.data, packet.size, nil);
	while (av_fifo_size(audio_fifo) >= audio_output_frame_size) {
		av_fifo_generic_read(audio_fifo, audio_buffer, audio_output_frame_size, nil);

		av_init_packet(&pkt);
		c = st->codec;

		//get_audio_frame(samples, audio_input_frame_size, c->channels);

		frame->nb_samples = audio_input_frame_size;
		

		if (avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt, (uint8_t *)audio_buffer, audio_output_frame_size, 0) < 0) {
			assert(0);
		}
		/*
		avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
									(uint8_t *)buffer, //samples,
									audio_input_frame_size *
									av_get_bytes_per_sample(c->sample_fmt) *
									c->channels, 1);
									*/

		avcodec_encode_audio2(c, &pkt, frame, &got_packet);
		if (!got_packet) {		
			traceL("AVEncoder") << "Audio failed: XXXXXXXXXXXXXXXXXXXXXXXXXX: " << got_packet << endl;
			return;
		}
		traceL("AVEncoder") << "Audio Success^^^^^^^^^^^: " << audio_output_frame_size << endl;

		pkt.stream_index = st->index;
		
		pkt.flags |= AV_PKT_FLAG_KEY;
		if (pkt.pts != AV_NOPTS_VALUE)
			pkt.pts      = av_rescale_q(pkt.pts,      c->time_base, st->time_base);
		if (pkt.dts != AV_NOPTS_VALUE)
			pkt.dts      = av_rescale_q(pkt.dts,      c->time_base, st->time_base);
		if (pkt.duration > 0)
			pkt.duration = av_rescale_q(pkt.duration, c->time_base, st->time_base);
		

		traceL("AVEncoder") << "AUDIOOOOOOOOOOOOOOOO:" 
			<< "\n\tPTS: " << pkt.pts
			<< "\n\tDTS: " << pkt.dts
			/*
			<< "\n\tTime: " << time
			<< "\n\tDelta: " << delta
			<< "\n\tNumber: " << ((float) st->time_base.den / (float) st->time_base.num / (float) 1000000)
			<< "\n\tCalc PTS: " << _audioPts
			<< "\n\tDuration: " << pkt.duration
			//<< "\n\tTimecoden: " << _audio->frame->timecode
			*/
			<< "\n\tAudio Stream Calc PTS: " << (double)st->pts.val * st->time_base.num / st->time_base.den
			<< "\n\tAudio Stream PTS: " << st->pts.val << ": " << st->pts.den << ": " << st->pts.num
			<< "\n\tStream Time Base: " << st->time_base.den << ": " << st->time_base.num
			<< "\n\tCodec Time Base: " << c->time_base.den << ": " << c->time_base.num
			<< endl;

		// Write the compressed frame to the media file. 
		if (av_interleaved_write_frame(oc, &pkt) != 0) {
			fprintf(stderr, "Error while writing audio frame\n");
			assert(0);
		}
	}

    avcodec_free_frame(&frame);
}

static void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);

    //av_free(samples);
}

/**************************************************************/
// video output */

static AVFrame *picture, *tmp_picture;
static int frame_count;


static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return nil;
    size        = avpicture_get_size((PixelFormat)pix_fmt, width, height);
    picture_buf = (uint8_t *)av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return nil;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   (PixelFormat)pix_fmt, width, height);
    return picture;
}

static void open_video(AVFormatContext *oc, AVStream *st,  AVCodec *codec)
{
    AVCodecContext *c;

    c = st->codec;

    // open the codec */
    if (avcodec_open2(c, codec, nil) < 0) {
        fprintf(stderr, "could not open codec\n");
        assert(0);
    }

    // Allocate the encoded raw picture. 
    picture = alloc_picture((AVPixelFormat)c->pix_fmt, c->width, c->height);
    if (!picture) {
        fprintf(stderr, "Could not allocate picture\n");
        assert(0);
    }

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format. 
    tmp_picture = nil;
    if (c->pix_fmt != PIX_FMT_YUV420P) {
        tmp_picture = alloc_picture((AVPixelFormat)PIX_FMT_YUV420P, c->width, c->height);
        if (!tmp_picture) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            assert(0);
        }
    }
}

// Add a video output stream. 
static AVStream *add_video_stream(AVFormatContext *oc, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;
    AVCodec *codec;

    // find the video encoder */	
	//codec = avcodec_find_encoder_by_name("mpeg4");
	codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        assert(0);
    }

    st = avformat_new_stream(oc, codec);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        assert(0);
    }

    c = st->codec;

    // Put sample parameters. 
    c->bit_rate = 400000;
    // Resolution must be a multiple of two. 
    c->width    = 352;
    c->height   = 288;
    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1. 
    c->time_base.den = STREAM_FRAME_RATE;
    c->time_base.num = 1;
    c->gop_size      = 12; // emit one intra frame every twelve frames at most */
    c->pix_fmt       = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        // just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        // Needed to avoid using macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane. 
        c->mb_decision = 2;
    }
    // Some formats want stream headers to be separate. 
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	
	open_video(oc, st, codec);

    return st;
}

// Prepare a dummy image. 
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    // Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    // Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

/*

traceL("AVEncoder", this) << "********************************************************************: audio_pts: " << audio_pts << endl;
} 
else {
traceL("AVEncoder", this) << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@: video_pts: " << video_pts << endl;

cv::VideoCapture __cap(0);
//write_video_frame(oc, _video->stream);
cv::Mat frame;
__cap >> frame;
MatPacket vpacket(&frame);
encodeVideo((unsigned char*)vpacket.data(), vpacket.size(), vpacket.width, vpacket.height, vpacket.time);
*/


static void write_video_frame(AVFormatContext *oc, AVStream *st, unsigned char* buffer, int bufferSize)
{
    int ret;
    AVCodecContext *c;
    static struct SwsContext *img_convert_ctx;

    c = st->codec;
			
	//picture->data[0] = buffer;

    if (frame_count >= STREAM_NB_FRAMES) {
		
		traceL("AVEncoder") << "TOO MANY FRAMES!!!: " << frame_count << endl;
        // No more frames to compress. The codec has a latency of a few
        // frames if using B-frames, so we get the last frames by
        // passing the same picture again. 
    } else {
        if (c->pix_fmt != PIX_FMT_YUV420P) {
            // as we only generate a YUV420P picture, we must convert it
            // to the codec pixel format if needed */
            if (img_convert_ctx == nil) {
                img_convert_ctx = sws_getContext(c->width, c->height,
                                                 PIX_FMT_YUV420P,
                                                 c->width, c->height,
                                                 c->pix_fmt,
                                                 sws_flags, nil, nil, nil);
                if (img_convert_ctx == nil) {
                    fprintf(stderr,
                            "Cannot initialize the conversion context\n");
                    assert(0);
                }
            }
            fill_yuv_image(tmp_picture, frame_count, c->width, c->height);
            sws_scale(img_convert_ctx, tmp_picture->data, tmp_picture->linesize,
                      0, c->height, picture->data, picture->linesize);
        } else {
            fill_yuv_image(picture, frame_count, c->width, c->height);
			//assert(0);
        }
    }

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - the API will change slightly in the near
        // future for that. 
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = st->index;
        pkt.data          = (uint8_t *)picture;
        pkt.size          = sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt = { 0 };
        int got_packet;
        av_init_packet(&pkt);

        // encode the image */
        ret = avcodec_encode_video2(c, &pkt, picture, &got_packet);
        // If size is zero, it means the image was buffered. 
        if (!ret && got_packet && pkt.size) {
            if (pkt.pts != AV_NOPTS_VALUE) {
                pkt.pts = av_rescale_q(pkt.pts,
                                       c->time_base, st->time_base);
            }
            if (pkt.dts != AV_NOPTS_VALUE) {
                pkt.dts = av_rescale_q(pkt.dts,
                                       c->time_base, st->time_base);
            }
            pkt.stream_index = st->index;

            // Write the compressed frame to the media file. 
            ret = av_interleaved_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        fprintf(stderr, "Error while writing video frame\n");
        assert(0);
    }
    frame_count++;
}

static void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    //av_free(picture->data[0]);
    av_free(picture);
    if (tmp_picture) {
        //av_free(tmp_picture->data[0]);
        av_free(tmp_picture);
    }
}


/**************************************************************/
// Encoder Struct */
struct EncoderRef {	
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st, *video_st;
    double audio_pts, video_pts;
};


/**************************************************************/
// Audio Callback */


int audioCallback( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *data)
{
	EncoderRef* ref = (EncoderRef*)data;

	
	//_audioQueue.push(pkt);
	
	if (frame_count == 0) {				
        //write_video_frame(oc, _video->stream);
		//cv::Mat frame;
		__cap >> __frame;
	}	
	
	// Compute current audio and video time. 
	if (ref->audio_st)
		ref->audio_pts = (double)ref->audio_st->pts.val * ref->audio_st->time_base.num / ref->audio_st->time_base.den;
	else
		ref->audio_pts = 0.0;

	if (ref->video_st)
		ref->video_pts = (double)ref->video_st->pts.val * ref->video_st->time_base.num / ref->video_st->time_base.den;
	else
		ref->video_pts = 0.0;
	
		//traceL("AVEncoder") << "ref->video_st: " << ref->video_st << endl;
		//traceL("AVEncoder") << "ref->video_st 1: " << (!ref->video_st || ref->video_pts >= STREAM_DURATION) << endl;
		//traceL("AVEncoder") << "ref->audio_pts 1: " << (!ref->audio_st || ref->audio_pts >= STREAM_DURATION) << endl;
		//traceL("AVEncoder") << "ref->audio_pts 1: " << (ref->audio_pts >= STREAM_DURATION) << endl;
	if ((!ref->audio_st || ref->audio_pts >= STREAM_DURATION) &&
		(!ref->video_st || ref->video_pts >= STREAM_DURATION)) {
		traceL("AVEncoder") << "EXITINGGGGGGGGGGGGGGGGGGG" << endl;
		return 1;
	}

	// write interleaved audio and video frames
	if (!ref->video_st || (ref->video_st && ref->audio_st && ref->audio_pts < ref->video_pts)) {
		traceL("AVEncoder") << "********************************************************************: audio_pts: " << ref->audio_pts << endl;
		traceL("AVEncoder") << "*****************************************: streamTime: " << streamTime << endl;
		
		/*
		// Generated
		APacket pkt;
		pkt.size = (audio_input_frame_size * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2);
		pkt.data = samples; //new int16_t[pkt.size + 1];		
		get_audio_frame(pkt.data, audio_input_frame_size, 2);	
		*/	

		//traceL("AVEncoder") << "av_get_bytes_per_sample: " << av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) << endl;
		//traceL("AVEncoder") << "sizeof( MY_TYPE ): " << sizeof( MY_TYPE ) << endl;

		//samples

		// = (audio_input_frame_size * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2);
		
		APacket pkt;
		pkt.size = nBufferFrames * 2 * sizeof( MY_TYPE );
		pkt.data =  (MY_TYPE *) inputBuffer;
		pkt.time = streamTime;
		//pkt.data = new int16_t[pkt.size + 1];
		//std::memcpy(pkt.data, inputBuffer, pkt.size);
		
		//APacket& packet = _audioQueue.back();
		//uint8_t * buffer = (uint8_t *)packet.array();
		//int size = packet.size();
		write_audio_frame(ref->oc, ref->audio_st, pkt);
		//write_audio_frame(ref->oc, ref->audio_st, buffer, (nBufferFrames * 2 * 2));
		//_audioQueue.pop();

		//write_audio_frame(ref->oc, ref->audio_st, buffer, (nBufferFrames * 2 * 2));
	} else {
		traceL("AVEncoder") << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@: video_pts: " << ref->video_pts << endl;
		//write_video_frame(ref->oc, ref->video_st, __frame.data, __frame.rows*__frame.step);
	}

		
	/*
	_audioQueue.push(AudioPacket((char*)inputBuffer, 
		nBufferFrames * 2 * 2,
		(double)streamTime));
	
	traceL("AVEncoder") << "Audio: Time: " << streamTime << endl;
	
	*/
	
	/*
	// Note: do nothing here for pass through.
	double *my_buffer = (double *) buffer;

	// Scale input data for output.
	for (int i=0; i<bufferSize; i++) {
	// Do for two channels.
	*my_buffer++ *= 0.5;
	*my_buffer++ *= 0.5;
	}
	*/

	return 0;
}


/**************************************************************/
// Encoder Thread */

static void runThread(void* arg)
{
	EncoderRef* ref = (EncoderRef*)arg;
	
	for (;;) {
		if ((!ref->audio_st || ref->audio_pts >= STREAM_DURATION) &&
			(!ref->video_st || ref->video_pts >= STREAM_DURATION)) {
			traceL("AVEncoder") << "EXITINGGGGGGGGGGGGGGGGGGG" << endl;
			return;
		}
	}

	/*	
	if (frame_count == 0) {				
        //write_video_frame(oc, _video->stream);
		//cv::Mat frame;
		__cap >> __frame;
	}


	for (;;) {
	
		// Compute current audio and video time. 
		if (ref->audio_st)
			ref->audio_pts = (double)ref->audio_st->pts.val * ref->audio_st->time_base.num / ref->audio_st->time_base.den;
		else
			ref->audio_pts = 0.0;

		if (ref->video_st)
			ref->video_pts = (double)ref->video_st->pts.val * ref->video_st->time_base.num /
						ref->video_st->time_base.den;
		else
			ref->video_pts = 0.0;

		if ((!ref->audio_st || ref->audio_pts >= STREAM_DURATION) &&
			(!ref->video_st || ref->video_pts >= STREAM_DURATION)) {
			traceL("AVEncoder") << "EXITINGGGGGGGGGGGGGGGGGGG" << endl;
			return;
		}

		// write interleaved audio and video frames
		if (!ref->video_st || (ref->video_st && ref->audio_st && ref->audio_pts < ref->video_pts)) {
			traceL("AVEncoder") << "********************************************************************: audio_pts: " << ref->audio_pts << endl;
			
			//std::deque<AudioPacket> _audioQueue;

			APacket& packet = _audioQueue.back();
			//uint8_t * buffer = (uint8_t *)packet.array();
			//int size = packet.size();
			write_audio_frame(ref->oc, ref->audio_st, packet);
			//write_audio_frame(ref->oc, ref->audio_st, buffer, (nBufferFrames * 2 * 2));
			_audioQueue.pop();

			//write_audio_frame(ref->oc, ref->audio_st, buffer, (nBufferFrames * 2 * 2));
		} else {
			traceL("AVEncoder") << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@: video_pts: " << ref->video_pts << endl;
			write_video_frame(ref->oc, ref->video_st, __frame.data, __frame.rows*__frame.step);
		}

	}
	*/
}


/**************************************************************/
// media file output

/*
int main(int argc, char **argv)
{
	Logger::instance().add(new ConsoleChannel("debug", TraceLevel));
    const char *filename;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st, *video_st;
    //double audio_pts, video_pts;
	video_st = nil;
    int i;

    // Initialize libavcodec, and register all codecs and formats. 
    av_register_all();

	filename = "fftest.mp4";
	filename = "fftest.mp3";

    // Autodetect the output format from the name. default is MPEG. 
    fmt = av_guess_format(nil, filename, nil);
	fmt->video_codec = AV_CODEC_ID_NONE;
    //if (!fmt) {
    //    printf("Could not deduce output format from file extension: using MPEG.\n");
    //    fmt = av_guess_format("mpeg", nil, nil);
    //}
    if (!fmt) {
        fprintf(stderr, "Could not find suitable output format\n");
		

		std::puts("Press any key to continue...");
		std::getchar();
        return 1;
    }

    // Allocate the output media context. 
    oc = avformat_alloc_context();
    if (!oc) {
        fprintf(stderr, "Memory error\n");
		std::puts("Press any key to continue...");
		std::getchar();
        return 1;
    }
    oc->oformat = fmt;
    _snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

    // Add the audio and video streams using the default format codecs
    // and initialize the codecs. 
    video_st = nil;
    audio_st = nil;
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
		//fmt->video_codec = AV_CODEC_ID_H264;
        video_st = add_video_stream(oc, fmt->video_codec);
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		//fmt->audio_codec = AV_CODEC_ID_AAC;
        audio_st = add_audio_stream(oc, fmt->audio_codec);
    }

    av_dump_format(oc, 0, filename, 1);

    // open the output file, if needed 
    if (!(fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
			

			std::puts("Press any key to continue...");
			std::getchar();
            return 1;
        }
    }

    // Write the stream header, if any. 
    avformat_write_header(oc, nil);


	//
	// Init Audio
	//

RtDeviceManager::initialize();
  RtAudio adc;

	 // Let RtAudio print messages to stderr.
	  adc.showWarnings( true );

	  // Set our stream parameters for input only.
	  unsigned int bufferFrames = 256; //audio_input_frame_size; //512;
	  RtAudio::StreamParameters iParams;
	  iParams.deviceId = 4;
	  iParams.nChannels = 2;
	  iParams.firstChannel = 0;
  
		// Set the stream callback function
		EncoderRef* ref = new EncoderRef;
		ref->fmt = fmt;
		ref->oc = oc;
		ref->video_st = video_st;
		ref->audio_st = audio_st;
		ref->video_pts = 0;
		ref->audio_pts = 0;

  //InputData data;
  //data.buffer = 0;
  try {

    adc.openStream( nil, &iParams, FORMAT, 44100, &bufferFrames, audioCallback, (void *)ref );
  }
  catch ( RtError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
  }


  try {
    adc.startStream();
  }
  catch ( RtError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
  }
  

  


	//
	// Run and join thread
	//
	
	//traceL("AVEncoder") << "Run Thread" << endl;
	//Thread _encoderThread;
	//_encoderThread.start(runThread, ref);
	//traceL("AVEncoder") << "Run Thread OK" << endl;
	//_encoderThread.join();
	//traceL("AVEncoder") << "Run Thread Exit" << endl;

	std::puts("Press any key to continue...");
	std::getchar();
	
    adc.stopStream();


    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close(). 
    av_write_trailer(oc);

    // Close each codec. 
    if (video_st)
        close_video(oc, video_st);
    if (audio_st)
        close_audio(oc, audio_st);

    // Free the streams. 
    for (i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE))
        // Close the output file. 
        avio_close(oc->pb);

    // free the stream
    av_free(oc);
	

	std::puts("Press any key to continue...");
	std::getchar();
	Logger::shutdown();
    return 0;
} 
*/
	/*
    for (;;) {
        // Compute current audio and video time. 
        if (audio_st)
            audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
        else
            audio_pts = 0.0;

        if (video_st)
            video_pts = (double)video_st->pts.val * video_st->time_base.num /
                        video_st->time_base.den;
        else
            video_pts = 0.0;

        if ((!audio_st || audio_pts >= STREAM_DURATION) &&
            (!video_st || video_pts >= STREAM_DURATION))
            break;

        // write interleaved audio and video frames
        if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
            write_audio_frame(oc, audio_st);
        } else {
            write_video_frame(oc, video_st);
        }
    }
	*/
