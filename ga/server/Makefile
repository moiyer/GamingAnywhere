
TARGET	= periodic event-posix

all:
	for t in $(TARGET); do make -C $$t; done

install:
	for t in $(TARGET); do make -C $$t install; done

clean:
	for t in $(TARGET); do make -C $$t clean; done

