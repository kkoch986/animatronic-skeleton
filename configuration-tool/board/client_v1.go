package board

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log/slog"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

type messageType byte

const (
	MOVE              messageType = 0x01
	SET               messageType = 0x02
	MOVEALL           messageType = 0x03
	SETALL            messageType = 0x04
	SETMIN            messageType = 0x05
	SETMAX            messageType = 0x06
	SETIDLE           messageType = 0x07
	COMMIT            messageType = 0x08
	RESTORE           messageType = 0x09
	QUERY_MOTOR_STATE messageType = 0x0A
	SETENABLED        messageType = 0x0B
	SETLABEL          messageType = 0x0C
)

type stateRestoreStatus byte

const (
	SRS_OK                   stateRestoreStatus = 0x00
	SRS_INVALID_STATE_HEADER stateRestoreStatus = 0x01
)

type V1WebSocketClient struct {
	// lock will be used to lock the connection when we
	// write a message that expects a response from the client
	// this ensure that the wires dont get crossed
	lock sync.Mutex
	conn *websocket.Conn
}

func NewV1WebSocketClient(conn *websocket.Conn) Client {
	return &V1WebSocketClient{conn: conn}
}

func (v *V1WebSocketClient) write(m []byte) error {
	err := v.conn.WriteMessage(websocket.BinaryMessage, m)
	if err != nil {
		return err
	}
	return nil
}

type addrPong struct {
	Address int `json:"address"`
}

// RemoteAddr will return the remote address of the board
func (v *V1WebSocketClient) RemoteAddr() string {
	return v.conn.RemoteAddr().String()
}

// SetCloseHandler will set a function to be called when the connection
// should be closed
func (v *V1WebSocketClient) SetCloseHandler(f func(code int, text string) error) {
	v.conn.SetCloseHandler(f)
}

// Close will close the connection
func (v *V1WebSocketClient) Close() error {
	return v.conn.Close()
}

// QueryAddress will send a message to the board that triggers it
// to respond with its address
func (v *V1WebSocketClient) QueryAddress(ctx context.Context) (byte, error) {
	v.lock.Lock()
	defer v.lock.Unlock()
	// send a ping to get the address back
	err := v.conn.WriteControl(websocket.PingMessage, []byte{}, time.Now().Add(5*time.Second))
	if err != nil {
		return 0, err
	}
	mt, p, err := v.conn.ReadMessage()
	if err != nil {
		return 0, err
	}
	if mt != websocket.TextMessage {
		// LATER: handle this better by burning through other messages?
		panic("expected text message")
	}
	resp := addrPong{}
	err = json.Unmarshal(p, &resp)
	if err != nil {
		return 0, err
	}
	addr := byte(resp.Address)
	return addr, nil
}

// QueryMotorState will send a message to the board that triggers it
// to reply with the current state of each of its indexable motors
func (v *V1WebSocketClient) QueryMotorState(ctx context.Context, addr byte) ([]MotorState, error) {
	v.lock.Lock()
	defer v.lock.Unlock()

	err := v.write([]byte{addr, byte(QUERY_MOTOR_STATE)})
	if err != nil {
		return nil, err
	}

	mt, p, err := v.conn.ReadMessage()
	if err != nil {
		return nil, err
	}
	if mt != websocket.BinaryMessage {
		// LATER: handle this better by burning through other messages?
		panic("expected binary message")
	}
	slog.Debug("message received", "message", fmt.Sprintf("%08b", p), "len", len(p))

	if p[0] != byte(QUERY_MOTOR_STATE) {
		slog.Warn("unexpected message type", "expected", QUERY_MOTOR_STATE, "got", p[0])
		return nil, errors.New("unexpected message type")
	}

	// parse the message into a state
	body := p[1:]
	motorCount := int(body[0])
	labelMaxSize := int(body[1])
	bytesPerMotor := 9 + labelMaxSize
	slog.Debug(
		"state dump header info",
		"count", motorCount,
		"labelMaxSize", labelMaxSize,
		"body", fmt.Sprintf("%08b", body),
	)
	ret := make([]MotorState, motorCount)
	for i := 0; i < motorCount; i++ {
		start := 2 + (i * bytesPerMotor)
		labelLength := int(body[start+8])
		ret[i] = MotorState{
			Enabled:         body[start] == 1,
			LowerLimit:      body[start+1],
			UpperLimit:      body[start+2],
			IdlePosition:    body[start+3],
			SmoothingFactor: body[start+4],
			CurrentPosition: body[start+5],
			TargetPosition:  body[start+6],
			Arrived:         body[start+7] == 1,
			Label:           string(p[start+10 : start+10+labelLength]),
		}
	}
	slog.Debug("finished parsing state dump", "states", ret)

	return ret, nil
}

// MOVE is used to set the position of a particular servo (or light)
// when MOVE is used, the value will be travelled to via the smoothing
// parameters and not move immediately to the desired position.
// This should be the preferred way to adjust motor position during
// performances since it prevents high-speed, jerky motion from happening
func (v *V1WebSocketClient) Move(
	ctx context.Context,
	addr byte,
	motor byte,
	val byte,
) error {
	v.lock.Lock()
	defer v.lock.Unlock()
	return v.write([]byte{addr, byte(MOVE), motor, val})
}

// SET works like move, but will bypass any software based smoothing
func (v *V1WebSocketClient) Set(ctx context.Context, addr byte, motor byte, val byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()
	return v.write([]byte{addr, byte(SET), motor, val})
}

// MOVEALL works like move but will set a value for all of the servos
// if less than 16 value bytes are provided, will fill up to the point that
// is provided and not modify any others
func (v *V1WebSocketClient) MoveAll(ctx context.Context, addr byte, vals []byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	m := make([]byte, 2+len(vals))
	m[0] = addr
	m[1] = byte(MOVEALL)
	for i, v := range vals {
		m[i+2] = v
	}
	return v.write(m)
}

// SETALL works like MOVEALL but will bypass any software based smoothing
func (v *V1WebSocketClient) SetAll(ctx context.Context, addr byte, vals []byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	m := make([]byte, 2+len(vals))
	m[0] = addr
	m[1] = byte(SETALL)
	for i, v := range vals {
		m[i+2] = v
	}
	return v.write(m)
}

// SETMIN will set the minimum PWM range for the given motor
// val should be a single byte which will map 0 - 255 onto the range
// of 100 - PCA9685_PWM_FULL (2^16~=65k)
func (v *V1WebSocketClient) SetMin(ctx context.Context, addr byte, motor byte, val byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	return v.write([]byte{addr, byte(SETMIN), motor, val})
}

// SETMAX will set the maximum PWM range for the given motor
// works the same as SETMIN
func (v *V1WebSocketClient) SetMax(ctx context.Context, addr byte, motor byte, val byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	return v.write([]byte{addr, byte(SETMAX), motor, val})
}

// SETIDLE will set the idle position for the given motor
// val should be a single byte which will map 0 - 255 onto the range of
// [-90,90]
func (v *V1WebSocketClient) SetIdle(ctx context.Context, addr byte, motor byte, val byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	return v.write([]byte{addr, byte(SETIDLE), motor, val})
}

// COMMIT will commit the current settings to the board EEPROM
func (v *V1WebSocketClient) Commit(ctx context.Context, addr byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	return v.write([]byte{addr, byte(COMMIT)})
}

// RESTORE will restore the settings from the EEPROM
func (v *V1WebSocketClient) Restore(ctx context.Context, addr byte) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	err := v.write([]byte{addr, byte(RESTORE)})
	if err != nil {
		return err
	}

	mt, p, err := v.conn.ReadMessage()
	if err != nil {
		return err
	}
	if mt != websocket.BinaryMessage {
		panic("expected binary message")
	}
	slog.Debug("message received", "message", fmt.Sprintf("%08b", p), "len", len(p))

	if p[0] != byte(RESTORE) {
		slog.Warn("unexpected message type", "expected", RESTORE, "got", p[0])
		return errors.New("unexpected message type")
	}

	// parse the message into a state
	statusCode := stateRestoreStatus(p[1])
	if statusCode != SRS_OK {
		return fmt.Errorf("restore failed: %v", statusCode)
	}
	return nil
}

// SetEnabled will enable or disable the motor
func (v *V1WebSocketClient) SetEnabled(ctx context.Context, addr byte, motor byte, enabled bool) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	val := byte(0)
	if enabled {
		val = 1
	}
	return v.write([]byte{addr, byte(SETENABLED), motor, val})
}

// SetLabel will set the label for the motor
// [ADDR][OP][SERVO_INDEX][LABEL_SIZE][LABEL]
func (v *V1WebSocketClient) SetLabel(ctx context.Context, addr byte, motor byte, label string) error {
	v.lock.Lock()
	defer v.lock.Unlock()

	m := []byte{
		addr,
		byte(SETLABEL),
		motor,
		byte(len(label)),
	}
	m = append(m, []byte(label)...)
	return v.write(m)
}
