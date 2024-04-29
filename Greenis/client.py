import redis
import time

r = redis.Redis(host="127.0.0.1", port=8080)

print(r.get("forever"))

print(r.set("forever", "value32"))

print(r.set("key", "value", ex=10))

for i in range(12):
    print(r.get("key"))
    time.sleep(1)

print(r.get("non-existing-key"))

print(r.get("forever"))