# `std::shared_ptr`

`std::shared_ptr` is a smart pointer used when **multiple parts of a program need to share ownership of the same object**.

It uses **reference counting** to manage the object's lifetime.

```cpp
auto p1 = std::make_shared<Node>();
auto p2 = p1;
```

Now both `p1` and `p2` own the same `Node`.

```text
p1 ──┐
     ├──> Node
p2 ──┘
```

When one `shared_ptr` is destroyed or reset, the reference count decreases. The object is destroyed automatically when the **last `shared_ptr` is gone**.

## When to use it

Use `shared_ptr` when:

* Multiple components genuinely need to **own** the same object.
* The lifetime of the object cannot be tied to one clear owner.
* An object needs to remain alive while readers/workers are still using it.

### Example: concurrent readers

```cpp
std::shared_ptr<MemTable> immutable = current_memtable;
```

Even if another thread replaces or resets `current_memtable`, the `MemTable` remains alive because `immutable` still owns it.

## Important distinction

`shared_ptr` is about **ownership**, not simply passing pointers around.

* `unique_ptr` → one owner
* `shared_ptr` → multiple owners
* raw pointer/reference → usually non-owning access

**Rule of thumb:** Prefer `unique_ptr` when there is one clear owner. Use `shared_ptr` only when ownership is genuinely shared.
