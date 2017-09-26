package main

import (
	"flag"
	"fmt"
	"log"
	"net"
)

var (
	sockFile = flag.String("file", "/tmp/xyz", "dummy file bound to unix socket")
	mode     = flag.String("mode", "SERVER_SPEAK", "operation mode {SERVER_SPEAK}")
)

func main() {
	flag.Parse()

	// setup unix socket to transmit frames
	l, err := net.ListenPacket("unixgram", *sockFile)
	if err != nil {
		log.Fatalln(err)
	}
	log.Printf("listening on %s", *sockFile)

	var barf [4]byte
	n, addr, err := l.ReadFrom(barf[:])
	fmt.Println(n, addr, err)

	var i byte
	i = 0
	for {
		var data [1024]byte
		data[0] = i
		i += 1
		l.WriteTo(data[:], addr)
	}
}
