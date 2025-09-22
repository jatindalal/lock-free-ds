#include <atomic>
#include <optional>
#include <mutex>
#include <thread>
#include <unordered_set>

/*
    1) Single Threaded Stack
*/
template<typename T>
class StackSingleThreaded {
public:
    ~StackSingleThreaded() {
        while (head) { Node *n = head; head = head->next; delete n; }
    }
    void push(const T& v) {
        Node *n = new Node(v);
        n->next = head;
        head = n;
    }
    std::optional<T> pop() {
        if (!head) {
            return std::nullopt;
        }

        Node *n = head;
        head = head->next;
        T val = std::move(n->value);
        delete n;
        return val;
    }
private:
    struct Node {
        T value;
        Node *next;
        Node(const T& v) : value(v), next(nullptr) {}
    };
    Node *head = nullptr;
};

/*
    2) Locked (mutex) stack
*/
template <typename T>
class StackLocked {
    ~StackLocked() {
        std::lock_guard<std::mutex> lk(m);
        while (head) { Node *n = head; head = head->next; delete n; }
    }
    void push(const T& v) {
        std::lock_guard<std::mutex> lk(m);
        Node *n = new Node(v);
        n->next = head;
        head = n;
    }
    std::optional<T> pop() {
        std::lock_guard<std::mutex> lk(m);
        if (!head) {
            return std::nullopt;
        }

        Node *n = head;
        head = head->next;
        T val = std::move(n->value);
        delete n;
        return val;
    }
private:
    struct Node {
        T value;
        Node *next;
        Node(const T& v) : value(v), next(nullptr) {}
    };
    Node *head = nullptr;
    std::mutex m;
};

/*
    3) Treiber lock-free stack (CAS)
       NOTE: This version does NOT reclaim node memory safely
       Deleting nodes here can be unsafe without hazard pointers
*/
template <typename T>
class StackTreiber {
    ~StackTreiber() {
        Node *p = head.load();
        while (head) { Node *n = head; head = head->next; delete n; }
    }
    void push(const T& v) {
        Node *n = new Node(v);
        n->next = head.load(std::memory_order_relaxed);
        // loop in attempt to CAS head from old->n->next to n
        while (!head.compare_exchange_weak(n->next, n,
                                           std::memory_order_release,
                                           std::memory_order_relaxed)) {
            // n->next gets updated to current head by compare_exchange_weak on failure
        }
    }
    std::optional<T> pop() {
        Node *old = head.load(std::memory_order_relaxed);
        while (old) {
            Node *next = old->next;
            if (head.compare_exchange_weak(old, next,
                                           std::memory_order_acq_rel,
                                           std::memory_order_relaxed)) {
                /*
                 * CAUTION: deleting old here is unsafe in general, because
                 * another thread might sitll hold al pointer to old.
                 */
                T val = std::move(old->value);
                // delete old; // <-- unsafe without safe reclamation
                return val;
            }

            // else old is updated to current head; loop
        }
        return std::nullopt;
    }
private:
    struct Node {
        T value;
        Node *next;
        Node(const T& v) : value(v), next(nullptr) {}
    };
    std::atomic<Node*> head{nullptr};
};

/*
    4) Simple Hazard Pointer Manager
       - fixed max slots
       - each thread acquired a slot (once) and uses it to protect a poitner
       - retire-list per thread; periodic scanning reclaims nodes not in hazard
         list
*/
class HazardPointerManager {
public:
    // Get a thred-local slot index (allocate on first call)
    static int acquire_slot_index() {

    }
    // Return reference to atomic pointer for current thread's slot
    static std::atomic<void*>& get_hazard_for_current_thread() {

    }
    // Retire an object pointer; `deleter` knows how to delete it (typed)
    static void retire(void* p, const std::function<void(void*)>& deleter) {
        static thread_local std::vector<void*> retired;
        retired.push_back(p);
        if (retired.size() >= RECLAIM_THRESHOLD) {
            reclaim(retired, deleter);
        }
    }
private:
    static constexpr int MAX_HAZARD_POINTERS = 128;
    static constexpr size_t RECLAIM_THRESHOLD = 64;

    struct Slot {
        std::atomic<void*> ptr;
        bool used;
        Slot() : ptr(nullptr), used(false) {}
    };

    inline static Slot slots[MAX_HAZARD_POINTERS];
    inline static std::mutex alloc_mutex;

    static void reclaim(std::vector<void*>& retired, const std::function<void(void*)>& deleter) {
        // gather hazard pointers snapshot
        std::unordered_set<void*> hazards;
        hazards.reserve(MAX_HAZARD_POINTERS);
        
        for (int i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            void *hp = slots[i].ptr.load(std::memory_order_acquire);
            if (hp) hazards.insert(hp);
        }

        // Partition retuired: delete those not in hazards, keep the rest
    }
};

// template <typename T>
// class StackTreiber {
//     ~StackTreiber() {
//         Node *p = head.load();
//         while (head) { Node *n = head; head = head->next; delete n; }
//     }
//     void push(const T& v) {
//         Node *n = new Node(v);
//         n->next = head.load(std::memory_order_relaxed);
//         // loop in attempt to CAS head from old->n->next to n
//         while (!head.compare_exchange_weak(n->next, n,
//                                            std::memory_order_release,
//                                            std::memory_order_relaxed)) {
//             // n->next gets updated to current head by compare_exchange_weak on failure
//         }
//     }
//     std::optional<T> pop() {
//         Node *old = head.load(std::memory_order_relaxed);
//         while (old) {
//             Node *next = old->next;
//             if (head.compare_exchange_weak(old, next,
//                                            std::memory_order_acq_rel,
//                                            std::memory_order_relaxed)) {
//                 /*
//                  * CAUTION: deleting old here is unsafe in general, because
//                  * another thread might sitll hold al pointer to old.
//                  */
//                 T val = std::move(old->value);
//                 // delete old; // <-- unsafe without safe reclamation
//                 return val;
//             }

//             // else old is updated to current head; loop
//         }
//         return std::nullopt;
//     }
// private:
//     struct Node {
//         T value;
//         Node *next;
//         Node(const T& v) : value(v), next(nullptr) {}
//     };
//     std::atomic<Node*> head{nullptr};
// };