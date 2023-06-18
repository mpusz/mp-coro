#include <mp-coro/async.h>
#include <mp-coro/cancellation.h>
#include <mp-coro/sync_await.h>
#include <mp-coro/task.h>
#include <iostream>
#include <thread>

mp_coro::task<int> to_be_cancelled(mp_coro::cancellation_token c_token) {
  using namespace std::chrono_literals;
  c_token.throw_if_cancellation_requested();
  co_return 0;
}

int main()
{
  auto c_source = mp_coro::cancellation_source{};
  auto t = to_be_cancelled(c_source.token());
  c_source.request_cancellation();
  try {
    auto ret = mp_coro::sync_await(t);
    std::cout << "Result: " << ret << "\n";
  } catch (const std::exception& ex) {
    std::cout << "Exception caught: " << ex.what() << "\n";
  }
  return 0;
}
