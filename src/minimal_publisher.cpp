// Minimal Pub/Sub sample using FastDDS
// Inspired by the HelloWorld example from FastDDS 

#include "MinimalPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <time.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/attributes/BuiltinTransports.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/TCPv6TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

#define SHM_SEGMENT_SIZE 1920*1280*10
#define UDP_BUF_SIZE 1920*1280*10
#define SHM_TRANSPORT 1         // Uses data sharing as well (check wqos.data_sharing().automatic();) 
#define UDP_TRANSPORT 0
#define LARGE_TRANSPORT 0
#define SLEEP_TIME_MS 5000

class MinimalPublisher
{
    private:
        Minimal minimal_;
        DomainParticipant* participant_;
        Publisher* publisher_;
        Topic* topic_;
        DataWriter* writer_;
        TypeSupport type_;

    class PubListener : public DataWriterListener
    {
        public:
            PubListener()
                : matched_(0)
            {
            }

            ~PubListener() override
            {
            }

            void on_publication_matched(
                    DataWriter*,
                    const PublicationMatchedStatus& info) override
            {
                if (info.current_count_change == 1)
                {
                    matched_ = info.total_count;
                    std::cout << "Publisher matched." << std::endl;
                }
                else if (info.current_count_change == -1)
                {
                    matched_ = info.total_count;
                    std::cout << "Publisher unmatched." << std::endl;
                }
                else
                {
                    std::cout << info.current_count_change
                            << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
                }
            }

            std::atomic_int matched_;

    }listner_;

    public:
        MinimalPublisher()
            : participant_(nullptr)
            , publisher_(nullptr)
            , topic_(nullptr)
            , writer_(nullptr)
            , type_(new MinimalPubSubType())
        {
        }

        virtual ~MinimalPublisher()
        {
            if (writer_ != nullptr)
            {
                publisher_->delete_datawriter(writer_);
            }
            if (publisher_ != nullptr)
            {
                participant_->delete_publisher(publisher_);
            }
            if (topic_ != nullptr)
            {
                participant_->delete_topic(topic_);
            }
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }

        //!Initialize the publisher
        bool init()
        {
            // Explicitly create the shared memory transport
            DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
            pqos.name("Participant_pub");
            pqos.transport().use_builtin_transports = false;

            #if SHM_TRANSPORT
            std::shared_ptr<SharedMemTransportDescriptor> shm_transport =
                    std::make_shared<SharedMemTransportDescriptor>();
            shm_transport->segment_size(SHM_SEGMENT_SIZE);                  // Tuned:10 images of 1920x1280 pixels
            
            pqos.transport().user_transports.push_back(shm_transport);
            #endif

            #if UDP_TRANSPORT
            auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
            udp_transport->sendBufferSize = UDP_BUF_SIZE;
            udp_transport->receiveBufferSize = UDP_BUF_SIZE;
            udp_transport->non_blocking_send = true;
            pqos.transport().user_transports.push_back(udp_transport);
            #endif

            #if LARGE_DATA
            pqos.transport().use_builtin_transports = true;
            pqos.setup_transports(BuiltinTransports::LARGE_DATA);
            #endif  

            minimal_.index(0);
            minimal_.time_stamp(0);
            // minimal_.img_data(NULL);
            
            // Create the participant
            participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);

            if (participant_ == nullptr)
            {
                return false;
            }

            // Register the type
            type_.register_type(participant_);

            // Create the publications Topic
            topic_ = participant_->create_topic("MinimalTopic", "Minimal", TOPIC_QOS_DEFAULT);

            if (topic_ == nullptr)
            {
                return false;
            }

            // Create the Publisher
            publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

            if (publisher_ == nullptr)
            {
                return false;
            }

            // Create the DataWriter
            #if SHM_TRANSPORT
            DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
            wqos.data_sharing().automatic();
            #endif

            writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listner_);

            if (writer_ == nullptr)
            {
                return false;
            }
            return true;
        }

        //!Send a publication
        bool publish(std::ifstream& file, size_t size)
        {
            // Allocate memory for the data
            minimal_.img_data().resize(size);
            file.read((char*)minimal_.img_data().data(), size);
            struct timeval tv;

            if (listner_.matched_ > 0)
            {
                minimal_.index(minimal_.index() + 1);
                gettimeofday(&tv, NULL);
                auto time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
                minimal_.time_stamp(time);
                writer_->write(&minimal_);
                return true;
            }
            return false;
        }

        //!Run the Publisher
        void run(uint32_t samples)
        {
            uint32_t samples_sent = 0;
            const std::string path = "/home/dungrup/ext-vol/fastdds-minimal-sample/src/img.png";

            // Read the image data from the file
            std::ifstream file(path);
            if (!file.is_open())
            {
                std::cout << "Error opening file" << std::endl;
            }

            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            while (samples_sent < samples)
            {
                if (publish(file, size))
                {
                    samples_sent++;
                    std::cout << "[" << minimal_.time_stamp() <<"] Image with index: " << minimal_.index()
                                << " SENT" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
            }

            file.close();
        }
};

int main(int argc, char** argv)
{
    std::cout << "Starting MinimalPublisher" << std::endl;
    int samples = 10;

    MinimalPublisher* mypub = new MinimalPublisher();
    if (mypub->init())
    {
        mypub->run(samples);
    }

    delete mypub;
    return 0;
}