#include "Proxy.hpp"

using namespace cshr;

void execute(ITarget &target) {
    target.execute();
}
int main() {
  auto target = std::make_shared<Target>();
  auto proxy_cshr = Proxy(target);
  auto proxy_visitor = Proxy(target);
  proxy_cshr.login("cshr", "");
  proxy_visitor.login("test", "");
  ITarget &cshr = proxy_cshr, &vis = proxy_visitor;
  execute(cshr);
  execute(vis);
}