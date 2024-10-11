/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package main

import (
	"context"
	"flag"
	"os"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	corev1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	clientgoscheme "k8s.io/client-go/kubernetes/scheme"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"
	zapcr "sigs.k8s.io/controller-runtime/pkg/log/zap"

	dwsv1alpha2 "github.com/DataWorkflowServices/dws/api/v1alpha2"
	nnfv1alpha3 "github.com/NearNodeFlash/nnf-sos/api/v1alpha3"
)

var (
	scheme = runtime.NewScheme()
)

func init() {
	utilruntime.Must(clientgoscheme.AddToScheme(scheme))
	utilruntime.Must(nnfv1alpha3.AddToScheme(scheme))
	utilruntime.Must(dwsv1alpha2.AddToScheme(scheme))
}

func main() {

	var sysConfig string = "default"
	var thing string

	flag.StringVar(&thing, "thing", "other-thing", "A thingy.")

	flag.Parse()

	encoder := zapcore.NewConsoleEncoder(zap.NewDevelopmentEncoderConfig())
	zaplogger := zapcr.New(zapcr.Encoder(encoder), zapcr.UseDevMode(true))

	ctrl.SetLogger(zaplogger)
	myLog := ctrl.Log.WithName("dm-usher")

	config := ctrl.GetConfigOrDie()

	client, err := client.New(config, client.Options{Scheme: scheme})
	if err != nil {
		myLog.Error(err, "Unable to create client")
		os.Exit(1)
	}

	systemConfig := &dwsv1alpha2.SystemConfiguration{}
	if err := client.Get(context.TODO(), types.NamespacedName{Name: sysConfig, Namespace: corev1.NamespaceDefault}, systemConfig); err != nil {
		myLog.Error(err, "Failed to retrieve system config")
		os.Exit(1)
	}
	myLog.Info("Found system config", "UID", systemConfig.UID)
}
