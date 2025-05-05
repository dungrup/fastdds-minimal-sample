# Fastdds minimal-sample
This repository implements a simple FastDDS publisher and subscriber to transport images. [minimal_publisher.cpp](./src/minimal_publisher.cpp) implements a publisher that reads img.png and pushes it to the Minimal topic. [minimal_subscriber.cpp](./src/minimal_subscriber.cpp) subscribes to the topic and writes it to dest_dir.

## Pre-requisites

- Install FastDDS and FastDDSGen following [these](https://fast-dds.docs.eprosima.com/en/latest/installation/sources/sources_linux.html) steps

## How to use this

- The [Minimal.idl](./src/Minimal.idl) defines the message data structure for this example. We will need to generate the associated support files for this message type by using FastDDSGen. (If you want to define your own .idl file)

```
<path/to/Fast DDS-Gen>/scripts/fastddsgen Minimal.idl
```

- Publishing frequency is defined in minimal_publisher.cpp as a macro SLEEP_TIME_MS. Currently set to 100 hence all the published data may not be received by the subscriber. Tune this value accordingly. 

- Number of samples transmitted by the publisher is defined in the variable 'samples' of minimal_publisher.cpp. If you change this, vary change the same variable in minimal_subscriber.cpp as well.

- You may switch between transmitting an actual PNG image or create your own dummy data of variable sizes. Use the macro IMG_TRANSFER to toggle this and vary DATA_SIZE to experiment with different sizes

- Now build the source files:

```
cmake ..
cmake --build .
```

- Run the executables in separate terminals
```
./DDSMinimalPublisher
./DDSMinimalSubscriber
```

## Updates

02/10/2025: Samples now have macros to enable/disable different transport methods
05/05/2025: Latency measurement is fixed. Updated Readme.