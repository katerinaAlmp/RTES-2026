#ifndef CONFIG_H
#define CONFIG_H

#define BUFFER_SIZE 64
#define MESSAGE_SIZE 65536
#define LOG_FILE "metrics_log.txt"

#define WS_ADDRESS "jetstream1.us-east.bsky.network"
#define WS_PORT 443
#define WS_PATH "/subscribe?wantedCollections=app.bsky.feed.post"

#endif