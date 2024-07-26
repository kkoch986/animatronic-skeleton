package server

import (
	"context"
	"errors"
	"log/slog"
	"sync"
	"time"

	"github.com/kkoch986/animatronic-skeletons/configuration-tool/board"
)

const PingInterval = time.Second * 10

type Board struct {
	Client         board.Client
	RemoteAddr     string
	FirstActive    time.Time
	LastPing       time.Time
	MotorStateAsOf time.Time
	Motors         []board.MotorState
}

type ConnectionMap struct {
	lock        *sync.RWMutex
	connections map[byte]map[string]*Board
}

func NewConnectionMap() ConnectionMap {
	return ConnectionMap{
		lock:        &sync.RWMutex{},
		connections: map[byte]map[string]*Board{},
	}
}

var (
	ErrBoardNotFound = errors.New("board not found")
)

func (c ConnectionMap) All() map[byte]map[string]*Board {
	c.lock.RLock()
	defer c.lock.RUnlock()

	ret := make(map[byte]map[string]*Board)
	for k, v := range c.connections {
		ret[k] = make(map[string]*Board)
		for k2, v2 := range v {
			ret[k][k2] = v2
		}
	}
	return ret
}

func (c ConnectionMap) Get(addr byte, remoteAddr string) (*Board, error) {
	c.lock.RLock()
	defer c.lock.RUnlock()
	return c.get(addr, remoteAddr)
}

func (c ConnectionMap) get(addr byte, remoteAddr string) (*Board, error) {
	m, ok := c.connections[addr]
	if !ok {
		return nil, ErrBoardNotFound
	}
	r, ok := m[remoteAddr]
	if !ok {
		return nil, ErrBoardNotFound
	}
	return r, nil
}

func (c ConnectionMap) SetMotorState(addr byte, remoteAddr string, m []board.MotorState) error {
	c.lock.Lock()
	defer c.lock.Unlock()
	board, err := c.get(addr, remoteAddr)
	if err != nil {
		return err
	}
	board.Motors = m
	return nil
}

func (c ConnectionMap) Add(ctx context.Context, client board.Client) error {
	c.lock.Lock()
	defer c.lock.Unlock()

	remoteAddr := client.RemoteAddr()
	addr, err := client.QueryAddress(ctx)
	if err != nil {
		return err
	}
	slog.Debug("new client address", "addr", addr, "remoteAddr", remoteAddr)

	_, ok := c.connections[addr]
	if !ok {
		c.connections[addr] = map[string]*Board{}
	}
	c.connections[addr][remoteAddr] = &Board{
		Client:      client,
		RemoteAddr:  remoteAddr,
		LastPing:    time.Now(),
		FirstActive: time.Now(),
		Motors:      []board.MotorState{},
	}

	pollTimer := time.NewTicker(PingInterval)

	// handle removing closed connections
	client.SetCloseHandler(func(code int, text string) error {
		slog.Debug("closing connection", "remoteAddr", remoteAddr)
		pollTimer.Stop()
		return c.Remove(ctx, addr, remoteAddr)
	})

	// set up connection pinger
	go func() {
		for {
			select {
			case <-ctx.Done():
				slog.Warn("context done, closing connection")
				pollTimer.Stop()
				err = client.Close()
				slog.Error("error closing connection", "error", err)
			case <-pollTimer.C:
				slog.Debug("pinging connection", "addr", remoteAddr)
				_, err = client.QueryAddress(ctx)
				if err != nil {
					// LATER: are there some kinds of errors that shouldnt warrant closing the connection?
					slog.Debug("error pinging connection", "error", err)
					slog.Debug("closing connection", "addr", remoteAddr)
					err = client.Close()
					if err != nil {
						slog.Error("error closing connection", "error", err)
					}
					pollTimer.Stop()
					err = c.Remove(ctx, addr, remoteAddr)
					if err != nil {
						slog.Error("error removing connection", "error", err)
					}
				} else {
					c.connections[addr][remoteAddr].LastPing = time.Now()
				}
			}
		}
	}()

	return nil
}

func (c ConnectionMap) Remove(ctx context.Context, addr byte, remoteAddr string) error {
	c.lock.Lock()
	defer c.lock.Unlock()

	for _, conns := range c.connections {
		delete(conns, remoteAddr)
	}
	return nil
}
