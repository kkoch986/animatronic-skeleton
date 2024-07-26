package server

import (
	"log/slog"
	"net/http"
	"strconv"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
	"github.com/kkoch986/animatronic-skeletons/configuration-tool/board"
)

type Router struct {
	connections ConnectionMap
}

func NewServerRouter() *Router {
	return &Router{
		connections: NewConnectionMap(),
	}
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func (s *Router) RegisterRoutes(r gin.IRoutes) {
	r.GET("/", s.websocketHandler)
	r.GET("/config", s.index)

	// Partial endpoints
	r.GET("/connections", s.connectionList)
	r.GET("/connections/:addr/:remoteAddr", s.manageConnection)
	r.POST("/connections/:addr/:remoteAddr", s.updateConnection)
	r.GET("/connections/:addr/:remoteAddr/:motorIndex", s.manageConnectionMotor)
	r.PUT("/connections/:addr/:remoteAddr/:motorIndex", s.updateMotor)
}

func (s *Router) Shutdown() {
	for _, conns := range s.connections.All() {
		for _, c := range conns {
			err := c.Client.Close()
			if err != nil {
				slog.Error("failed to close connection", "error", err)
			}
		}
	}
}

type ConnectionUpdate struct {
	Address    byte   `uri:"addr"`
	RemoteAddr string `uri:"remoteAddr"`
	Action     string `form:"action"`
}

func (s *Router) updateConnection(ctx *gin.Context) {
	r := ConnectionUpdate{}
	err := ctx.BindUri(&r)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	err = ctx.Bind(&r)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}
	slog.Debug("update motor", "binding", r)

	conn, err := s.connections.Get(r.Address, r.RemoteAddr)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	switch r.Action {
	case "commit":
		err := conn.Client.Commit(ctx, r.Address)
		if err != nil {
			_ = ctx.AbortWithError(http.StatusInternalServerError, err)
		}
	case "restore":
		err := conn.Client.Restore(ctx, r.Address)
		if err != nil {
			_ = ctx.AbortWithError(http.StatusInternalServerError, err)
		}
	default:
		ctx.AbortWithStatus(http.StatusBadRequest)
	}

	s.manageConnection(ctx)
}

type MotorUpdate struct {
	Address      byte   `uri:"addr"`
	RemoteAddr   string `uri:"remoteAddr"`
	MotorIndex   byte   `uri:"motorIndex"`
	Action       string `form:"action"`
	NumericValue byte   `form:"numericValue"`
	StringValue  string `form:"stringValue"`
}

func (s *Router) updateMotor(ctx *gin.Context) {
	r := MotorUpdate{}
	err := ctx.BindUri(&r)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	err = ctx.Bind(&r)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}
	slog.Debug("update motor", "binding", r)

	conn, err := s.connections.Get(r.Address, r.RemoteAddr)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	switch r.Action {
	case "move":
		slog.Debug("moving motor", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.NumericValue)
		err = conn.Client.Move(ctx, r.Address, r.MotorIndex, r.NumericValue)
	case "set":
		slog.Debug("setting motor", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.NumericValue)
		err = conn.Client.Set(ctx, r.Address, r.MotorIndex, r.NumericValue)
	case "lowerlimit":
		slog.Debug("setting lower limit", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.NumericValue)
		err = conn.Client.SetMin(ctx, r.Address, r.MotorIndex, r.NumericValue)
	case "upperlimit":
		slog.Debug("setting upper limit", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.NumericValue)
		err = conn.Client.SetMax(ctx, r.Address, r.MotorIndex, r.NumericValue)
	case "label":
		slog.Debug("setting label", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.StringValue)
		err = conn.Client.SetLabel(ctx, r.Address, r.MotorIndex, r.StringValue)
	case "enable":
		slog.Debug("enabling motor", "addr", r.Address, "remoteAddr", r.RemoteAddr, "motorIndex", r.MotorIndex, "value", r.StringValue)
		checked := r.StringValue == "on"
		err = conn.Client.SetEnabled(ctx, r.Address, r.MotorIndex, checked)
	default:
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}
	if err != nil {
		_ = ctx.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	time.Sleep(100 * time.Millisecond)

	state, err := conn.Client.QueryMotorState(ctx, r.Address)
	if err != nil {
		_ = ctx.AbortWithError(http.StatusInternalServerError, err)
	} else {
		_ = s.connections.SetMotorState(r.Address, r.RemoteAddr, state)
	}

	s.manageConnectionMotor(ctx)
}

func (s *Router) websocketHandler(ctx *gin.Context) {
	w, r := ctx.Writer, ctx.Request
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		slog.Error("error converting connection to websocket", "error", err)
		return
	}

	client := board.NewV1WebSocketClient(c)
	err = s.connections.Add(ctx, client)
	if err != nil {
		slog.Error("error adding connection", "error", err)
		return
	}
}

func (s *Router) index(ctx *gin.Context) {
	ctx.HTML(http.StatusOK, "index.html", gin.H{
		"connections": s.connections.All(),
	})
}

func (s *Router) connectionList(ctx *gin.Context) {
	ctx.HTML(http.StatusOK, "connections", s.connections.All())
}

func (s *Router) manageConnection(ctx *gin.Context) {
	rawAddr := ctx.Params.ByName("addr")

	// parse rawAddr into an integer then a byte
	i, err := strconv.ParseInt(rawAddr, 10, 8)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	remoteAddr := ctx.Params.ByName("remoteAddr")
	addr := byte(i)
	conn, err := s.connections.Get(addr, remoteAddr)
	start := time.Now()
	if err != nil {
		ctx.AbortWithStatus(http.StatusNotFound)
	} else {
		state, err := conn.Client.QueryMotorState(ctx, addr)
		if err != nil {
			_ = ctx.AbortWithError(http.StatusInternalServerError, err)
		} else {
			_ = s.connections.SetMotorState(addr, remoteAddr, state)
			ctx.HTML(http.StatusOK, "editConnection", gin.H{
				"address":    byte(i),
				"remoteAddr": remoteAddr,
				"motors":     state,
				"asOf":       start.Format("Jan 2 15:04:05 2006 MST"),
				"duration":   time.Since(start).Seconds(),
			})
		}
	}
}

func (s *Router) manageConnectionMotor(ctx *gin.Context) {
	rawAddr := ctx.Params.ByName("addr")
	remoteAddr := ctx.Params.ByName("remoteAddr")
	motorIndexStr := ctx.Params.ByName("motorIndex")

	// parse rawAddr into an integer then a byte
	i, err := strconv.ParseInt(rawAddr, 10, 8)
	if err != nil {
		slog.Error("failed to parse address", "error", err)
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	motorIndex, err := strconv.ParseInt(motorIndexStr, 10, 32)
	if err != nil {
		slog.Error("failed to parse motor index", "error", err)
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	addr := byte(i)
	conn, err := s.connections.Get(addr, remoteAddr)
	if err != nil {
		ctx.AbortWithStatus(http.StatusBadRequest)
		return
	}

	motor := conn.Motors[motorIndex]
	ctx.HTML(http.StatusOK, "editMotor", gin.H{
		"address":    byte(i),
		"remoteAddr": remoteAddr,
		"index":      motorIndex,
		"motor":      motor,
	})
}
