#define BUF_SIZE 32

typedef struct ae {
    char const * filename;
    int32_t lin_num;
} AssertionError;

//static uint32_t const BUF_SIZE = 32;

typedef struct cb {
    AssertionError buffer[BUF_SIZE];
    uint32_t head;
    uint32_t tail;
} CBuffer;

static CBuffer cbuffer = {0};

// prototype
void cbuffer_insert(AssertionError const* ae);

void cbuffer_insert(AssertionError const* ae) {
    cbuffer.buffer[cbuffer.tail] = *ae;
    cbuffer.tail = (cbuffer.tail+1) % BUF_SIZE;
}
