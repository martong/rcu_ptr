# versioned_shared_ptr

Imagine we have a collection and several reader and some writer threads on it.
It is a common mistake by some programmers to hold a lock until the collection is iterated on the reader thread.
Example

```c++
class X {
    std::vector<int> v;
    mutable std::mutex m;
public:
    int sum() const { // read operation
        std::lock_guard<std::mutex> lock{m};
        return std::accumulate(v.begin(), v.end(), 0);
    }
    void add(int i) { // write operation
        std::lock_guard<std::mutex> lock{m};
        v.push_back(i);
    }
};
```

The first idea to make it better is to have a shared_ptr and hold the lock only until that is copied by the reader or updated by the writer.
```c++
class X {
    std::shared_ptr<std::vector<int>> v;
    mutable std::mutex m;
public:
    int sum() const { // read operation
        std::shared_ptr<std::vector<int>> local_copy;
        {
            std::lock_guard<std::mutex> lock{m};
            local_copy = v;
        }
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        std::shared_ptr<std::vector<int>> local_copy;
        {
            std::lock_guard<std::mutex> lock{m};
            local_copy = v;
        }
        local_copy->push_back(i);
        {
            std::lock_guard<std::mutex> lock{m};
            v = local_copy;
        }
    }
};
```
Now, if there are two concurrent write operations than we might miss one update.
We'd need to check whether the other writer had done an update after the actual writer has loaded the local copy.
If it did then we should load the data again and try to do the update again.
This leads to the general idea of using an `atomic_compare_exchange` in a while loop.
So we could use an `atomic_shared_ptr` from C++17, but until then we have to settle for the free funtcion overloads for shared_ptr.
  
```c++
class X {
    std::shared_ptr<std::vector<int>> v;
    mutable std::mutex m;
public:
    int sum() const { // read operation
        auto local_copy = std::atomic_load(&v);
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        auto local_copy = std::atomic_load(&v);
        local_copy->push_back(i);
        auto exchange_result = false;
        while (!exchange_result) {
            // True if exchange was performed
            // If sp == sp_l (share ownership of the same pointer),
            //   assigns r into sp
            // If sp != sp_l, assigns sp into sp_l
            exchange_result =
                std::atomic_compare_exchange_strong(&v, &local_copy,
                                                    local_copy);
        }
    }
};
```

Also (regarding the write operation), since we are already in a while loop we can use `atomic_compare_exchange_weak`.
That can result in a performance gain on some platforms.
See http://stackoverflow.com/questions/25199838/understanding-stdatomiccompare-exchange-weak-in-c11

But nothing stops an other programmer (e.g. a naive maintainer of the code years later) to add a new reader operation, like this:
```c++
    int another_sum() const {
        return std::accumulate(v->begin(), v->end(), 0);
    }
```
This is definetly a race condition and a problem. 
And this is the exact reason why versioned_shared_ptr was created.
The goal is to provide a general higher level abstraction above atomic_shared_ptr.

```c++
class X {
    versioned_shared_ptr<std::vector<int>> v;
    mutable std::mutex m;
public:
    int sum() const { // read operation
        std::shared_ptr<const std::vector<int>> local_copy = v.read();
        return std::accumulate(local_copy->begin(), local_copy->end(), 0);
    }
    void add(int i) { // write operation
        // Do a deep copy of the container
        auto local_copy = std::make_shared<std::vector<int>>(*v.read());
        local_copy->push_back(i);
        v.write(local_copy);
    }
};
```
The read operation of `versioned_shared_ptr` returns a shared_ptr<const T> by value, therefore it is thread safe.
The write operation receives a `const shared_ptr<T>&` which will be the new shared_ptr after the `atomic_compare_exchange` is finished inside.
Consequently, the write operation needs to do a deep copy if it wants to preserve some elements of the original data.

// TODO
Therefore `versioned_shared_ptr` is not as performant as `atomic_shared_ptr` e.g. in a case of a simple push_back.
But there are cases when we update the entire data in a lock step (e.g. fetch a table from a database), and there are no small data updates.

