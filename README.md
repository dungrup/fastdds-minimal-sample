# Fastdds minimal-sample
This repository implements a simple FastDDS publisher and subscriber to transport images. [minimal_publisher.cpp](./src/minimal_publisher.cpp) implements a publisher that reads img.png and pushes it to the Minimal topic. [minimal_subscriber.cpp](./src/minimal_subscriber.cpp) subscribes to the topic and writes it to dest_dir.

## Pre-requisites

- Install FastDDS and FastDDSGen following [these](https://fast-dds.docs.eprosima.com/en/latest/installation/sources/sources_linux.html) steps

## How to use this

- The [Minimal.idl](./src/Minimal.idl) defines the message data structure for this example. We will need to generate the associated support files for this message type by using FastDDSGen. (If you want to define your own .idl file)

```
<path/to/Fast DDS-Gen>/scripts/fastddsgen Minimal.idl
```

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