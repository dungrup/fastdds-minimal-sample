// Minimal Pub/Sub sample using FastDDS
// Inspired by the HelloWorld example from FastDDS 

#include "MinimalPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <fstream>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

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
                if (reader->take_next_sample(&minimal_, &info) == eprosima::fastdds::dds::RETCODE_OK)
                {
                    if (info.valid_data)
                    {
                        samples_++;
                        std::cout << "Image with index: " << minimal_.index()
                                  << " RECEIVED." << std::endl;
                        
                        // Save the image data to a file
                        // std::string filename = "/home/dungrup/ext-vol/dds_ws/dest_dir/received_image_" + std::to_string(minimal_.index()) + ".png";

                        // std::ofstream file(filename, std::ios::binary);
                        // file.write((char*)minimal_.img_data().data(), minimal_.img_data().size());
                        // file.close();
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
            // Create the participant
            DomainParticipantQos pqos;
            pqos.name("Participant_subscriber");
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
            reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

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
                // std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

};

int main(int argc, char** argv)
{
    std::cout << "Starting subscriber." << std::endl;
    MinimalSubscriber subscriber;
    if (subscriber.init())
    {
        subscriber.run(15);
    }
    return 0;
}