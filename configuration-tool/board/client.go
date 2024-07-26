package board

import (
	"context"
)

type MotorState struct {
	Enabled         bool
	LowerLimit      byte
	UpperLimit      byte
	IdlePosition    byte
	SmoothingFactor byte
	CurrentPosition byte
	TargetPosition  byte
	Arrived         bool
	Label           string
}

type Client interface {
	// RemoteAddr will return the remote address of the board
	RemoteAddr() string

	// SetCloseHandler will set a function to be called when the connection
	// should be closed
	SetCloseHandler(func(code int, text string) error)

	// Close will close the connection
	Close() error

	// QueryAddress will send a message to the board that triggers it
	// to respond with its address
	QueryAddress(ctx context.Context) (byte, error)

	// QueryMotorState will send a message to the board that triggers it
	// to reply with the current state of each of the indexable motors
	QueryMotorState(ctx context.Context, addr byte) ([]MotorState, error)

	// Move is used to set the position of a particular servo (or light)
	// when MOVE is used, the value will be travelled to via the smoothing
	// parameters and not move immediately to the desired position.
	// This should be the preferred way to adjust motor position during
	// performances since it prevents high-speed, jerky motion from happening
	Move(ctx context.Context, addr byte, motor byte, val byte) error

	// Set works like move, but will bypass any software based smoothing
	Set(ctx context.Context, addr byte, motor byte, val byte) error

	// MoveAll works like move but will set a value for all of the servos
	// if less than 16 value bytes are provided, will fill up to the point that
	// is provided and not modify any others
	MoveAll(ctx context.Context, addr byte, vals []byte) error

	// SetAll works like MOVEALL but will bypass any software based smoothing
	SetAll(ctx context.Context, addr byte, vals []byte) error

	// SetMin will set the minimum PWM range for the given motor
	// val should be a single byte which will map 0 - 255 onto the range
	// of 100 - PCA9685_PWM_FULL (2^16~=65k)
	SetMin(ctx context.Context, addr byte, motor byte, val byte) error

	// SetMax will set the maximum PWM range for the given motor
	// works the same as SETMIN
	SetMax(ctx context.Context, addr byte, motor byte, val byte) error

	// SetIdle will set the idle position for the given motor
	// val should be a single byte which will map 0 - 255 onto the range of
	// [-90,90]
	SetIdle(ctx context.Context, addr byte, motor byte, val byte) error

	// SetEnabled will enable or disable the motor
	SetEnabled(ctx context.Context, addr byte, motor byte, enabled bool) error

	// SetLabel will set the label for the motor
	SetLabel(ctx context.Context, addr byte, motor byte, label string) error

	// Commit will commit the current settings to the board EEPROM
	Commit(ctx context.Context, addr byte) error

	// Restore will restore the settings from the EEPROM
	Restore(ctx context.Context, addr byte) error
}
