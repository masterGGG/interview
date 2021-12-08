//对std::atomic_int64_t进行了封装，内存补齐保证m_seq在一个缓存行中
class AtomicSequence
{
public:
    AtomicSequence(int64_t num = 0L) : m_seq(num) {};
    ~AtomicSequence() {};
    AtomicSequence(const AtomicSequence&) = delete;
    AtomicSequence(const AtomicSequence&&) = delete;
    void operator=(const AtomicSequence&) = delete;

    //default: std::memory_order _order = std::memory_order_seq_cst)
    void store(const int64_t val) {
        m_seq.store(val);
    }

    //default: std::memory_order _order = std::memory_order_seq_cst)
    int64_t load() {
        return m_seq.load();// _order);
    }

    //default: std::memory_order _order = std::memory_order_seq_cst)
    int64_t fetch_add(const int64_t increment) {
        return m_seq.fetch_add(increment);// _order);
    }

private:
    //两边都补齐，以保证_seq不会与其它变量共享一个缓存行
    char m_frontPadding[CACHELINE_PADDING_FOR_ATOMIC_INT64_SIZE];
    std::atomic<int64_t> m_seq;
    char m_backPadding[CACHELINE_PADDING_FOR_ATOMIC_INT64_SIZE];
};