#include <iostream>
#include <memory>
#include <vector>

template <class T>
concept CopyConstructable =
    std::constructible_from<T, T&> && std::convertible_to<T&, T> &&
    std::constructible_from<T, const T&> && std::convertible_to<const T&, T>;

class AnyType {
 private:
  struct Base {
    virtual Base *copy() const = 0;
    virtual ~Base() {}
  };

  template <CopyConstructable T>
  struct Derived : public Base {
    T value;
    Derived(const T &value) : value(value) {}
    Derived(T &&value) : value(std::move(value)) {}
    Base *copy() const override { return new Derived(value); }
    ~Derived() = default;
  };

  Base *pointer;

 public:
  AnyType() = default;

  template <typename T>
  AnyType(const T &values) : pointer{new Derived<T>(values)} {}

  AnyType(const AnyType &other) : pointer{other.pointer->copy()} {}

  ~AnyType() { delete pointer; }

  AnyType operator=(const AnyType &other) {
    pointer = other.pointer->copy();
    return *this;
  }

  template <CopyConstructable T>
  friend T AnyCast(AnyType &a);
};

template <CopyConstructable T>
T AnyCast(AnyType &a) {
  auto *point = dynamic_cast<AnyType::Derived<std::remove_reference_t<T>> *>(a.pointer);
  if (!point) {
    throw std::bad_cast();
  }
  return point->value;
}

class BaseFutureTask {
 public:
  ~BaseFutureTask() = default;
};

class EmptyType {};

class BaseTask {
 public:
  ~BaseTask() = default;
  virtual void calculate(){};
};

template <typename Function, CopyConstructable T = EmptyType, CopyConstructable U = EmptyType>
class Task : public BaseTask {
 private:
  bool isCalculated_ = false;
  Function function_;
  T value1_;
  U value2_;
  AnyType result_;

 public:
  Task(Function f, T v1 = EmptyType(), U v2 = EmptyType())
      : function_(f), value1_(v1), value2_(v2) {}

  template <CopyConstructable ResultType>
  ResultType getResult() {
    if (!isCalculated_) {
      calculate();
      isCalculated_ = true;
    }
    return AnyCast<ResultType>(result_);
  }

  void calculate() override {
    if (isCalculated_) return;
    if constexpr (std::is_same_v<T, EmptyType>) {
      result_ = function_();
    } else if constexpr (std::is_same_v<U, EmptyType>) {
      if constexpr (std::is_base_of_v<BaseFutureTask, T>) {
        result_ = function_(value1_.getResult());
      } else {
        result_ = function_(value1_);
      }
    } else {
      if constexpr (std::is_base_of_v<BaseFutureTask, T>) {
        if constexpr (std::is_base_of_v<BaseFutureTask, U>) {
          result_ = function_(value1_.getResult(), value2_.getResult());
        } else {
          result_ = function_(value1_.getResult(), value2_);
        }
      } else {
        if constexpr (std::is_base_of_v<BaseFutureTask, U>) {
          result_ = function_(value1_, value2_.getResult());
        } else {
          result_ = function_(value1_, value2_)
        }
      }
    }
  }
};

template <CopyConstructable ResultType, typename Function, CopyConstructable T, CopyConstructable U>
class FutureTask : BaseFutureTask {
 private:
  Task<Function, T, U> *ptr_;

 public:
  FutureTask<ResultType, Function, T, U>(Task<Function, T, U> *task_)
      : ptr_(task_) {}
  ResultType getResult() { return ptr_->template getResult<ResultType>(); }
};

class TTaskScheduler {
 private:
  std::vector<BaseTask *> data;

 public:
  TTaskScheduler() = default;
  ~TTaskScheduler() = default;
  TTaskScheduler(const TTaskScheduler &other) = default;

  template <typename Function, CopyConstructable... Args>
  Task<Function, Args...> add(Function function, Args... args) {
    Task<Function, Args...> temp_task = Task<Function, Args...>(function, args...);
    data.push_back(&temp_task);
    return temp_task;
  }

  template <CopyConstructable ResultType, typename Function, CopyConstructable... Args>
  FutureTask<ResultType, Function, Args...> getFutureResult(
      Task<Function, Args...> &task_) const {
    return FutureTask<ResultType, Function, Args...>(&task_);
  }

  template <CopyConstructable ResultType, typename Function, CopyConstructable... Args>
  ResultType getResult(Task<Function, Args...> &task_) {
    return task_.template getResult<ResultType>();
  }

  void executeAll() {
    for (auto &t : data) {
      t->calculate();
    }
  }
};