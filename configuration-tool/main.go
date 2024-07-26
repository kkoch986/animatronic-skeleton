package main

import (
	"context"
	"fmt"
	"html/template"
	"log"
	"log/slog"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/kkoch986/animatronic-skeletons/configuration-tool/server"
	sloggin "github.com/samber/slog-gin"
)

func initContext() (context.Context, context.CancelFunc) {
	sigs := make(chan os.Signal, 1)

	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)
	ctx, cancel := context.WithCancel(context.Background())

	go func() {
		<-sigs
		cancel()
	}()
	return ctx, cancel
}

func main() {
	ctx, cancel := initContext()
	defer cancel()

	logger := slog.New(slog.NewJSONHandler(os.Stdout, &slog.HandlerOptions{
		Level: slog.LevelDebug,
	}))
	slog.SetDefault(logger)

	gin.SetMode(gin.ReleaseMode)
	r := gin.New()
	r.Use(gin.Recovery())
	r.Use(sloggin.New(logger))

	// TODO: incorporate this into the router maybe?
	r.SetFuncMap(template.FuncMap{
		"checked": func(a bool) string {
			if a {
				return "checked"
			}
			return ""
		},
		"timeSince": func(d time.Time) string {
			if d.IsZero() {
				return "n/a"
			}
			// TODO: make this a little prettier
			return fmt.Sprintf("%0.1fs", time.Since(d).Seconds())
		},
	})
	r.LoadHTMLGlob("server/templates/*")

	s := server.NewServerRouter()
	s.RegisterRoutes(r)

	// start the server
	srv := &http.Server{
		Addr:    ":8888",
		Handler: r.Handler(),
	}
	go func() {
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			slog.Error("failed to start server", "error", err)
			cancel()
		}
	}()
	slog.Info("server started", "addr", srv.Addr)

	// wait for the context to terminate to signal that we should start shutting down
	<-ctx.Done()
	slog.Info("shutting down server")
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		if err := srv.Shutdown(ctx); err != nil {
			log.Fatal("Server Shutdown:", err)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		s.Shutdown()
	}()

	wg.Wait()
	slog.Info("server shutdown complete")
}

// // TODO: remove this test code
// var i byte
// for {
// 	//       const val = Math.floor(127 + (127 * Math.sin(i * Math.PI / 180)));
// 	i = (i + 1) % 255
// 	err := client.MoveAll(
// 		context.Background(),
// 		addr,
// 		[]byte{i, i, i, i, i, i, i, i, i, i, i, i, i, i, i, i},
// 	)
// 	if err != nil {
// 		slog.Error("error moving servos", "error", err)
// 		break
// 	}
// 	time.Sleep(10 * time.Millisecond)
// }
