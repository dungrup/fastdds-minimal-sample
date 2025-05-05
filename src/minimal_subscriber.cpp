// Minimal Pub/Sub sample using FastDDS
// Inspired by the HelloWorld example from FastDDS 

#include "MinimalPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <fstream>
#include <sys/time.h>
#include <time.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/attributes/BuiltinTransports.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/TCPv6TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.hpp>

#define SHM_SEGMENT_SIZE 1920*1280*10   // Tuned:10 images of 1920x1280 pixels
#define UDP_BUF_SIZE 1920*1280*10
#define SHM_TRANSPORT 1         // Uses data sharing as well (check rqos.data_sharing().automatic();) 
#define UDP_TRANSPORT 0
#define LARGE_TRANSPORT 0

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;


class MinimalSubscriber
{
    private:
        DomainParticipant* participant_;
        Subscriber* subscriber_;
        DataReader* reader_;
        Topic* topic_;
        TypeSupport type_;

    class SubListener : public DataReaderListener
    {
        public:

            Minimal minimal_;
            std::atomic_int samples_;
            SubListener()
                : samples_(0)
            {
            }

            ~SubListener() override
            {
            }

            void on_subscription_matched(
                    DataReader*,
                    const SubscriptionMatchedStatus& info) override
            {
                if (info.current_count_change == 1)
                {
                    std::cout << "Subscriber matched." << std::endl;
                }
                else if (info.current_count_change == -1)
                {
                    std::cout << "Subscriber unmatched." << std::endl;
                }
                else
                {
                    std::cout << info.current_count_change
                            << " is not a valid value for SubscriptionMatchedStatus current count change." << std::endl;
                }
            }

            void on_data_available(
                    DataReader* reader) override
            {
                SampleInfo info;
                struct timeval time_val;
                if (reader->take_next_sample(&minimal_, &info) == eprosima::fastdds::dds::RETCODE_OK)
                {
                    if (info.valid_data)
                    {
                        samples_++;
                        gettimeofday(&time_val, NULL);
                        unsigned long now = time_val.tv_usec;
                        auto latency = (now - minimal_.time_stamp()) / 1000.0;
                        std::cout << "[" << minimal_.time_stamp() <<"] Image with index: " << minimal_.index()
                                  << " RECEIVED, latency: " << latency << " ms" << std::endl;
                        
                        // Save the image data to a file
                        // std::string filename = "/home/dungrup/ext-vol/dds_ws/dest_dir/received_image_" + std::to_string(minimal_.index()) + ".png";

                        // std::ofstream file(filename, std::ios::binary);
                        // file.write((char*)minimal_.img_data().data(), minimal_.img_data().size());
                        // file.close();

                        // Save the latency to a file
                        std::ofstream latency_file_;
                        latency_file_.open("latency.csv", std::ofstream::out | std::ofstream::app);
                        latency_file_ << now << "," << minimal_.time_stamp() << std::endl;
                        latency_file_.close();
                    }
                }
            }
    }listener_;

    public:
        MinimalSubscriber()
            : participant_(nullptr)
            , subscriber_(nullptr)
            , reader_(nullptr)
            , topic_(nullptr)
            , type_(new MinimalPubSubType())
        {
        }

        virtual ~MinimalSubscriber()
        {
            if (reader_ != nullptr)
            {
                subscriber_->delete_datareader(reader_);
            }
            if (topic_ != nullptr)
            {
                participant_->delete_topic(topic_);
            }
            if (subscriber_ != nullptr)
            {
                participant_->delete_subscriber(subscriber_);
            }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }

        // Init subscriber
        bool init()
        {
            // Explicitly create the shared memory transport
            DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
            pqos.name("Participant_subscriber");
            pqos.transport().use_builtin_transports = false;

            #if SHM_TRANSPORT
            std::shared_ptr<SharedMemTransportDescriptor> shm_transport =
                    std::make_shared<SharedMemTransportDescriptor>();
            shm_transport->segment_size(SHM_SEGMENT_SIZE);               
            
            pqos.transport().user_transports.push_back(shm_transport);
            #endif

            #if UDP_TRANSPORT
            auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
            udp_transport->sendBufferSize = UDP_BUF_SIZE;
            udp_transport->receiveBufferSize = UDP_BUF_SIZE;
            udp_transport->non_blocking_send = true;
            pqos.transport().user_transports.push_back(udp_transport);
            #endif

            #if LARGE_TRANSPORT
            pqos.transport().use_builtin_transports = true;
            pqos.setup_transports(BuiltinTransports::LARGE_DATA);
            #endif  
            
            // Create the participant
            participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);

            if (participant_ == nullptr)
            {
                return false;
            }

            // Register the type
            type_.register_type(participant_);

            // Create the subscriptions Topic
            topic_ = participant_->create_topic("MinimalTopic", "Minimal", TOPIC_QOS_DEFAULT);

            if (topic_ == nullptr)
            {
                return false;
            }

            // Create the Subscriber
            subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

            if (subscriber_ == nullptr)
            {
                return false;
            }

            // Create the DataReader
            #if SHM_TRANSPORT
            DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
            subscriber_->get_default_datareader_qos(reader_qos);
            reader_qos.reliability().kind = ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
            reader_qos.durability().kind = DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS;
            reader_qos.data_sharing().automatic();
            #endif
            reader_ = subscriber_->create_datareader(topic_, reader_qos, &listener_);

            if (reader_ == nullptr)
            {
                return false;
            }

            return true;
        }

        //!Run the Subscriber
        void run(
                uint32_t samples)
        {
            while (listener_.samples_ < samples)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

};

int main(int argc, char** argv)
{
    std::cout << "Starting subscriber." << std::endl;
    MinimalSubscriber subscriber;
    int samples = 10;

    if (subscriber.init())
    {
        subscriber.run(samples);
    }
    return 0;
}