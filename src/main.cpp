#include <chrono>
#include <iostream>
#include <map>
#include <memory>

#include <Average.h>
#include <Player.h>
#include <Map.h>
#include <WindowManager.h>
#include <Raycaster.h>
#include <UDPReceiver.h>
#include <UDPSender.h>
#include <DoubleBuffer.h>
#include <util.h>

#include <thread>
#include <mutex>
#include <condition_variable>

struct ProgramArguments
{
    int screenWidth;
    int screenHeight;
    std::string ipsPath;
};

ProgramArguments parseArgs(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <screenWidth> <screenHeight> <ipsPath>" << std::endl;
        std::cerr << "  screenWidth: The width of the screen." << std::endl;
        std::cerr << "  screenHeight: The height of the screen." << std::endl;
        std::cerr << "  ipsPath: The path to the file containing the IP addresses and ports of the players." << std::endl;
        std::cerr << "Example: " << argv[0] << " 1920 1080 ips.txt" << std::endl;
        exit(1);
    }

    ProgramArguments args;
    args.screenWidth = std::stoi(argv[1]);
    args.screenHeight = std::stoi(argv[2]);
    args.ipsPath = argc == 4 ? argv[3] : "";
    return args;
}

int main(int argc, char *argv[])
{
    ProgramArguments args = parseArgs(argc, argv);
    const int screenWidth = args.screenWidth;
    const int screenHeight = args.screenHeight;

    std::vector<std::unique_ptr<UDPSender>> udpSenders;
    NetworkData data = parseIPs(args.ipsPath);
    UDPReceiver udpReceiver(data.listeningPort);
    for (auto ipPort : data.ipPorts)
        udpSenders.push_back(std::unique_ptr<UDPSender>(new UDPSender(ipPort.first, ipPort.second)));
    size_t nbPlayers = udpSenders.size();

    // Indexes used to identify other players
    int nextPlayerIndex = 0;
    std::map<std::string, int> playersIndexes; // Maps IP addresses and ports to player indexes

    Map map = Map::generateMap(nbPlayers);
    Player player({22, 11.5}, {-1, 0}, {0, 0.66}, 5, 3, map);
    DoubleBuffer doubleBuffer(screenWidth, screenHeight);
    WindowManager windowManager(doubleBuffer);
    Raycaster raycaster(player, doubleBuffer, map);

    std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now(), oldTime;

    Average fpsCounter(1.0);

    std::mutex sendMutex;
    std::condition_variable sendCondition;
    bool positionUpdated = false;
    bool stopSending = false;

    double positionToSendX = player.posX();
    double positionToSendY = player.posY();
    


    std::thread senderThread([&]()
                             {
    while (true)
    {
        double currentX;
        double currentY;

        {
            std::unique_lock<std::mutex> lock(sendMutex);
            sendCondition.wait(lock, [&]()
            {
                return positionUpdated || stopSending;
            });

            if (stopSending)
                break;

            currentX = positionToSendX;
            currentY = positionToSendY;
            positionUpdated = false;
        }

        for (auto &udpSender : udpSenders)
            udpSender->send(currentX, currentY);
    } });

    while (true)
    {
        raycaster.castFloorCeiling();
        raycaster.castWalls();
        raycaster.castSprites();

        doubleBuffer.swap();

        oldTime = time;
        time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = time - oldTime;
        double frameTime = elapsed.count();

        fpsCounter.update(1.0 / frameTime);
        std::cout << "\r" << std::to_string(int(fpsCounter.get())) << " FPS" << std::flush;

        windowManager.updateDisplay();
        windowManager.updateInput();

        double oldX = player.posX();
        double oldY = player.posY();

        unsigned int keys = windowManager.getKeysPressed();

        if (keys & WindowManager::KEY_UP)
            player.move(frameTime);
        if (keys & WindowManager::KEY_DOWN)
            player.move(-frameTime);
        if (keys & WindowManager::KEY_RIGHT)
            player.turn(-frameTime);
        if (keys & WindowManager::KEY_LEFT)
            player.turn(frameTime);

        bool moved = player.posX() != oldX || player.posY() != oldY;

        if (moved)
        {
            std::lock_guard<std::mutex> lock(sendMutex);
            positionToSendX = player.posX();
            positionToSendY = player.posY();
            positionUpdated = true;
            sendCondition.notify_one();
        }

        if (keys & WindowManager::KEY_ESC)
            break;

        // Receive other players' positions and update them
        for (size_t i = 0; i < nbPlayers; i++)
        {
            UDPData data = udpReceiver.receive();
            if (!data.valid)
                break;
            // Update the player's index if it is the first time we receive data from them
            if (playersIndexes.find(data.sender) == playersIndexes.end())
            {
                playersIndexes[data.sender] = nextPlayerIndex++;
                nextPlayerIndex %= nbPlayers;
            }
            int index = playersIndexes[data.sender];
            map.movePlayer(index, data.position.x(), data.position.y());
        }
    }

    {
        std::lock_guard<std::mutex> lock(sendMutex);
        stopSending = true;
        sendCondition.notify_one();
    }

    senderThread.join();
    return 0;
}
