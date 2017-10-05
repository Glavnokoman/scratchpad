package bufbuf

import (
	"fmt"
	"sync"
)

// Thread-safe kinda circular buffer for single producer and single consumer.
// Consumer always gets the latest complete chunk of data.
// Buffer is never full, old data is just overwritten, but may be empty if there was no completed push() after last pop()
// FIFO in a sense that the order of chunks is preserved, although some may get discarded.
type BufBuf struct {
	sync.Mutex
	Buf  [][]byte
	WrId int // bucket being (potentially) currently written to, this should never be read from
	RdId int // bucket being (potentially) currently read from, this should never be written to
	Size int // counts pushes() since last pop(). every pop() sets it to 0
}

func NewBufBuf(size int) (*BufBuf, error) {
	if size < 3 {
		return nil, fmt.Errorf("BufBuf min length is 3")
	}
	return &BufBuf{
		Buf:  make([][]byte, size),
		WrId: 0,
		RdId: -1,
		Size: 0}, nil
}

func (buf *BufBuf) push(data []byte) {
	buf.Buf[buf.WrId] = data // will that copy?!?!?!? should I care??!?!?!?

	buf.Lock()
	buf.WrId = (buf.WrId + 1) % len(buf.Buf)
	for buf.WrId == buf.RdId {
		buf.WrId = (buf.WrId + 1) % len(buf.Buf)
	}
	buf.Size += 1
	buf.Unlock()
}

func (buf *BufBuf) pop() ([]byte, bool) {
	buf.Lock()
	if buf.Size == 0 {
		buf.Unlock()
		return nil, false
	}
	newRdID := (buf.WrId + len(buf.Buf) - 1) % len(buf.Buf)
	for newRdID == buf.RdId {
		newRdID = (newRdID + len(buf.Buf) - 1) % len(buf.Buf)
	}
	buf.RdId = newRdID
	buf.Size = 0
	buf.Unlock()

	return buf.Buf[buf.RdId], true
}
